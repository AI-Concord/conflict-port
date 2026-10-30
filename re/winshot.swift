// Prints the CGWindowID of the first on-screen window owned by the named app.
import CoreGraphics
import Foundation
let name = CommandLine.arguments[1]
let list = CGWindowListCopyWindowInfo([.optionAll], kCGNullWindowID) as! [[String: Any]]
for w in list where (w[kCGWindowOwnerName as String] as? String) == name {
    if let b = w[kCGWindowBounds as String] as? [String: Any], let h = b["Height"] as? Double, h > 200, let id = w[kCGWindowNumber as String] as? Int { print(id); break }
}
