import AppKit
import MetalKit

// MARK: - Renderer

final class Renderer: NSObject, MTKViewDelegate {
    private let device: MTLDevice
    private let queue: MTLCommandQueue
    private let pipeline: MTLRenderPipelineState
    private let texture: MTLTexture
    private var pixels = [UInt8](repeating: 0, count: Int(ENGINE_WIDTH * ENGINE_HEIGHT) * 4)

    // EGA 640x350 was shown on a 4:3 monitor, so pixels are taller than wide.
    static let displayAspect: CGFloat = 4.0 / 3.0

    private static let shaderSource = """
    #include <metal_stdlib>
    using namespace metal;
    struct VOut { float4 pos [[position]]; float2 uv; };
    vertex VOut vmain(uint vid [[vertex_id]], constant float2 &scale [[buffer(0)]]) {
        float2 p[4] = { float2(-1,-1), float2(1,-1), float2(-1,1), float2(1,1) };
        VOut o; o.pos = float4(p[vid] * scale, 0, 1);
        o.uv = float2((p[vid].x + 1) * 0.5, (1 - p[vid].y) * 0.5);
        return o;
    }
    // Two looks. Sharp: "sharp bilinear", crisp pixel centres with only the seam between two game pixels blended.
    // Smooth (the default): xBR-style edge smoothing for low-resolution art. Diagonal edges between game pixels,
    // the stair-steps of the lettering included, are found from their neighbours and redrawn as straight lines
    // (45 degrees, or 2:1 either way); straight edges stay sharp. Four samples per screen pixel antialias the result.
    // The same algorithm is in the Windows shader (win/main.cpp), and in the Pool of Radiance port where it was
    // first written.
    struct FP { float2 ppt; float mode; float pad; };
    static float3 tx(texture2d<float> t, int x, int y) { return t.read(uint2(clamp(x, 0, int(t.get_width()) - 1), clamp(y, 0, int(t.get_height()) - 1))).rgb; }
    static float cdist(float3 a, float3 b) {
        float3 d = a - b;
        return 48 * abs(dot(d, float3(0.299, 0.587, 0.114))) + 7 * abs(dot(d, float3(-0.169, -0.331, 0.5))) + 6 * abs(dot(d, float3(0.5, -0.419, -0.081)));
    }
    static bool same(float3 a, float3 b) { return cdist(a, b) < 0.1; }
    static float3 xbr(texture2d<float> t, float2 s, float d) {
        int ix = int(floor(s.x)), iy = int(floor(s.y)); float fx = s.x - ix, fy = s.y - iy;
        int dx = fx < 0.5 ? -1 : 1, dy = fy < 0.5 ? -1 : 1;       // the nearest corner, mirrored to bottom-right
        if (dx < 0) fx = 1 - fx; if (dy < 0) fy = 1 - fy;
        float3 E = tx(t, ix, iy), F = tx(t, ix + dx, iy), H = tx(t, ix, iy + dy), I = tx(t, ix + dx, iy + dy);
        float3 B = tx(t, ix, iy - dy), D = tx(t, ix - dx, iy), C = tx(t, ix + dx, iy - dy), G = tx(t, ix - dx, iy + dy);
        float3 F4 = tx(t, ix + 2 * dx, iy), I4 = tx(t, ix + 2 * dx, iy + dy), H5 = tx(t, ix, iy + 2 * dy), I5 = tx(t, ix + dx, iy + 2 * dy);
        float e = cdist(E, C) + cdist(E, G) + cdist(I, F4) + cdist(I, H5) + 4 * cdist(H, F);
        float i = cdist(H, D) + cdist(H, I5) + cdist(F, I4) + cdist(F, B) + 4 * cdist(E, I);
        bool edge = e < i && !same(E, F) && !same(E, H) && (!(same(F, B) && same(F, C)) || !(same(H, D) && same(H, G)));
        if (!edge) return E;
        bool shallow = 2 * cdist(F, G) <= cdist(H, C) && !same(E, G) && !same(D, G);
        bool steep = 2 * cdist(H, C) <= cdist(F, G) && !same(E, C) && !same(B, C);
        float a = smoothstep(1.5 - d, 1.5 + d, fx + fy);
        if (shallow) a = max(a, smoothstep(2.0 - 1.5 * d, 2.0 + 1.5 * d, fx + 2 * fy));
        if (steep) a = max(a, smoothstep(2.0 - 1.5 * d, 2.0 + 1.5 * d, 2 * fx + fy));
        return mix(E, cdist(E, F) <= cdist(E, H) ? F : H, a);
    }
    fragment float4 fmain(VOut in [[stage_in]], texture2d<float> tex [[texture(0)]], constant FP &p [[buffer(0)]]) {
        float2 size = float2(tex.get_width(), tex.get_height());
        float2 t = in.uv * size;
        if (p.mode > 0.5) {
            float2 k = max(p.ppt, float2(1.0)), o = 0.25 / k;
            float d = 0.5 / min(k.x, k.y);
            float3 c = xbr(tex, t + float2(-o.x, -o.y), d) + xbr(tex, t + float2(o.x, -o.y), d) + xbr(tex, t + float2(-o.x, o.y), d) + xbr(tex, t + float2(o.x, o.y), d);
            return float4(c * 0.25, 1.0);
        }
        constexpr sampler s(filter::linear, address::clamp_to_edge);
        float2 i = floor(t), f = t - i;
        float2 k = max(p.ppt, float2(1.0));
        f = clamp((f - 0.5) * k + 0.5, 0.0, 1.0);
        return float4(tex.sample(s, (i + f) / size).rgb, 1.0);
    }
    """

    init?(view: MTKView) {
        guard let dev = view.device, let q = dev.makeCommandQueue() else { return nil }
        device = dev; queue = q
        do {
            let lib = try dev.makeLibrary(source: Renderer.shaderSource, options: nil)
            let d = MTLRenderPipelineDescriptor()
            d.vertexFunction = lib.makeFunction(name: "vmain")
            d.fragmentFunction = lib.makeFunction(name: "fmain")
            d.colorAttachments[0].pixelFormat = view.colorPixelFormat
            pipeline = try dev.makeRenderPipelineState(descriptor: d)
        } catch { NSLog("Metal setup failed: \(error)"); return nil }
        let td = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .bgra8Unorm, width: Int(ENGINE_WIDTH), height: Int(ENGINE_HEIGHT), mipmapped: false)
        td.usage = [.shaderRead]
        guard let tex = dev.makeTexture(descriptor: td) else { return nil }
        texture = tex
        super.init()
    }

    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}

    func draw(in view: MTKView) {
        if engine_has_exited() != 0 { NSApp.terminate(nil); return }
        pixels.withUnsafeMutableBytes { buf in
            _ = engine_copy_frame(buf.baseAddress!.assumingMemoryBound(to: UInt8.self))
            texture.replace(region: MTLRegionMake2D(0, 0, Int(ENGINE_WIDTH), Int(ENGINE_HEIGHT)), mipmapLevel: 0, withBytes: buf.baseAddress!, bytesPerRow: Int(ENGINE_WIDTH) * 4)
        }
        guard let pass = view.currentRenderPassDescriptor, let drawable = view.currentDrawable,
              let cmd = queue.makeCommandBuffer(), let enc = cmd.makeRenderCommandEncoder(descriptor: pass) else { return }
        let ds = view.drawableSize
        var scale = SIMD2<Float>(1, 1)
        let viewAspect = ds.width / max(ds.height, 1)
        if viewAspect > Renderer.displayAspect { scale.x = Float(Renderer.displayAspect / viewAspect) } else { scale.y = Float(viewAspect / Renderer.displayAspect) }
        var ppt = SIMD4<Float>(Float(ds.width) * scale.x / Float(ENGINE_WIDTH), Float(ds.height) * scale.y / Float(ENGINE_HEIGHT),
                               UserDefaults.standard.object(forKey: "SmoothPicture") as? Bool ?? true ? 1 : 0, 0)
        enc.setRenderPipelineState(pipeline)
        enc.setVertexBytes(&scale, length: MemoryLayout<SIMD2<Float>>.size, index: 0)
        enc.setFragmentBytes(&ppt, length: MemoryLayout<SIMD4<Float>>.size, index: 0)
        enc.setFragmentTexture(texture, index: 0)
        enc.drawPrimitives(type: .triangleStrip, vertexStart: 0, vertexCount: 4)
        enc.endEncoding()
        cmd.present(drawable)
        cmd.commit()
    }
}

// MARK: - Keyboard

/// Translates Mac key events to the BIOS scancode/ASCII pairs the game expects.
enum KeyMap {
    // macOS virtual key code -> BIOS scancode for non-character keys.
    static let special: [UInt16: UInt16] = [
        36: 0x1C0D, 76: 0x1C0D, 53: 0x011B, 48: 0x0F09, 51: 0x0E08, 49: 0x3920,
        126: 0x4800, 125: 0x5000, 123: 0x4B00, 124: 0x4D00,
        115: 0x4700, 119: 0x4F00, 116: 0x4900, 121: 0x5100, 117: 0x5300,
        122: 0x3B00, 120: 0x3C00, 99: 0x3D00, 118: 0x3E00, 96: 0x3F00, 97: 0x4000, 98: 0x4100, 100: 0x4200, 101: 0x4300, 109: 0x4400,
    ]
    // ASCII (lower-case) -> BIOS scancode.
    static let scan: [Character: UInt16] = {
        var m: [Character: UInt16] = [:]
        let rows: [(String, UInt16)] = [("1234567890-=", 0x02), ("qwertyuiop[]", 0x10), ("asdfghjkl;'`", 0x1E), ("\\zxcvbnm,./", 0x2B)]
        for (chars, start) in rows { for (i, c) in chars.enumerated() { m[c] = start + UInt16(i) } }
        return m
    }()
    static func bios(for e: NSEvent) -> UInt16? {
        if let s = special[e.keyCode] { return s }
        guard let ch = e.characters?.unicodeScalars.first, ch.isASCII, ch.value >= 32, ch.value < 127 else { return nil }
        let lower = Character(String(Character(ch)).lowercased())
        return ((scan[lower] ?? 0) << 8) | UInt16(ch.value)
    }
}

final class GameView: MTKView {
    override var acceptsFirstResponder: Bool { true }
    override func keyDown(with event: NSEvent) {
        if event.modifierFlags.contains(.command) { super.keyDown(with: event); return }
        if let k = KeyMap.bios(for: event) { engine_post_key(k) }
    }
    override func performKeyEquivalent(with event: NSEvent) -> Bool { false }
}

// MARK: - App

final class AppDelegate: NSObject, NSApplicationDelegate, NSWindowDelegate {
    @objc func setSmooth(_ sender: NSMenuItem) { setPicture(true, sender) }
    @objc func setSharp(_ sender: NSMenuItem) { setPicture(false, sender) }
    private func setPicture(_ smooth: Bool, _ sender: NSMenuItem) {
        UserDefaults.standard.set(smooth, forKey: "SmoothPicture")
        for item in sender.menu?.items ?? [] where item.action == #selector(setSmooth(_:)) || item.action == #selector(setSharp(_:)) {
            item.state = (item.action == #selector(setSmooth(_:))) == smooth ? .on : .off
        }
    }
    var window: NSWindow!
    var renderer: Renderer!

    func applicationDidFinishLaunching(_ note: Notification) {
        let fm = FileManager.default
        guard let gameDir = Bundle.main.resourceURL?.appendingPathComponent("GameData"), fm.fileExists(atPath: gameDir.path) else {
            fatal("The game data folder is missing from the application bundle."); return
        }
        let saveDir = fm.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0].appendingPathComponent("Conflict")
        try? fm.createDirectory(at: saveDir, withIntermediateDirectories: true)

        let content = NSRect(x: 0, y: 0, width: 960, height: 720)
        window = NSWindow(contentRect: content, styleMask: [.titled, .closable, .miniaturizable, .resizable], backing: .buffered, defer: false)
        window.title = "Conflict: Middle East Political Simulator"
        window.contentAspectRatio = NSSize(width: 4, height: 3)
        window.contentMinSize = NSSize(width: 640, height: 480)
        window.collectionBehavior = [.fullScreenPrimary]
        window.delegate = self
        window.center()

        guard let device = MTLCreateSystemDefaultDevice() else { fatal("Metal is not available on this Mac."); return }
        let view = GameView(frame: content, device: device)
        view.colorPixelFormat = .bgra8Unorm
        view.clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
        view.preferredFramesPerSecond = 60
        guard let r = Renderer(view: view) else { fatal("Could not set up the Metal renderer."); return }
        renderer = r
        view.delegate = r
        window.contentView = view
        window.makeKeyAndOrderFront(nil)
        window.makeFirstResponder(view)
        NSApp.activate(ignoringOtherApps: true)

        engine_start(gameDir.path, saveDir.path)
        scheduleTestKeys()
    }

    /// Test aid: CONFLICT_TEST_KEYS="6:enter,2:down,..." posts keys after the given delays (seconds).
    private func scheduleTestKeys() {
        guard let spec = ProcessInfo.processInfo.environment["CONFLICT_TEST_KEYS"] else { return }
        let codes: [String: UInt16] = ["enter": 0x1C0D, "down": 0x5000, "up": 0x4800, "end": 0x4F00, "home": 0x4700, "esc": 0x011B, "B": 0x3042, "S": 0x1F53, "E": 0x1245]
        var t = 0.0
        for item in spec.split(separator: ",") {
            let parts = item.split(separator: ":")
            guard parts.count == 2, let d = Double(parts[0]), let k = codes[String(parts[1])] else { continue }
            t += d
            DispatchQueue.main.asyncAfter(deadline: .now() + t) { engine_post_key(k) }
        }
    }
    func fatal(_ msg: String) {
        let a = NSAlert(); a.messageText = "Conflict cannot start"; a.informativeText = msg; a.runModal(); NSApp.terminate(nil)
    }
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }
    func applicationWillTerminate(_ note: Notification) { engine_request_quit() }
}

func buildMenu() {
    let main = NSMenu()
    let appItem = NSMenuItem(); main.addItem(appItem)
    let appMenu = NSMenu()
    appMenu.addItem(withTitle: "About Conflict", action: #selector(NSApplication.orderFrontStandardAboutPanel(_:)), keyEquivalent: "")
    appMenu.addItem(.separator())
    appMenu.addItem(withTitle: "Hide Conflict", action: #selector(NSApplication.hide(_:)), keyEquivalent: "h")
    appMenu.addItem(withTitle: "Quit Conflict", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")
    appItem.submenu = appMenu
    let viewItem = NSMenuItem(); main.addItem(viewItem)
    let viewMenu = NSMenu(title: "View")
    viewMenu.addItem(withTitle: "Enter Full Screen", action: #selector(NSWindow.toggleFullScreen(_:)), keyEquivalent: "f").keyEquivalentModifierMask = [.command, .control]
    viewMenu.addItem(.separator())
    let smooth = UserDefaults.standard.object(forKey: "SmoothPicture") as? Bool ?? true
    viewMenu.addItem(withTitle: "Smooth Picture", action: #selector(AppDelegate.setSmooth(_:)), keyEquivalent: "1").state = smooth ? .on : .off
    viewMenu.addItem(withTitle: "Sharp Pixels", action: #selector(AppDelegate.setSharp(_:)), keyEquivalent: "2").state = smooth ? .off : .on
    viewItem.submenu = viewMenu
    let winItem = NSMenuItem(); main.addItem(winItem)
    let winMenu = NSMenu(title: "Window")
    winMenu.addItem(withTitle: "Minimize", action: #selector(NSWindow.performMiniaturize(_:)), keyEquivalent: "m")
    winItem.submenu = winMenu
    NSApp.mainMenu = main
    NSApp.windowsMenu = winMenu
}

let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate
app.setActivationPolicy(.regular)
buildMenu()
app.run()
