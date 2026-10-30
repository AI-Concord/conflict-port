// Applies names and signatures from a text file so the decompiler output is readable.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.pcode.Varnode;
import ghidra.program.model.lang.Register;
import ghidra.program.model.symbol.SourceType;
import java.io.*;
import java.util.*;

public class ApplySigs extends GhidraScript {
    DataType dt(String s) {
        boolean ptr = s.endsWith("*"); if (ptr) s = s.substring(0, s.length() - 1);
        DataType b;
        switch (s) {
            case "double": b = DoubleDataType.dataType; break;
            case "float": b = FloatDataType.dataType; break;
            case "long": b = LongDataType.dataType; break;
            case "ulong": b = UnsignedLongDataType.dataType; break;
            case "uint": b = UnsignedShortDataType.dataType; break;
            case "char": b = CharDataType.dataType; break;
            case "uchar": b = ByteDataType.dataType; break;
            case "void": b = VoidDataType.dataType; break;
            default: b = ShortDataType.dataType;
        }
        return ptr ? new PointerDataType(b, 2) : b;
    }
    VariableStorage storage(String s, int size) throws Exception {
        if (s.startsWith("stack")) return new VariableStorage(currentProgram, Integer.parseInt(s.substring(5)), size);
        String[] rs = s.split(":"); Varnode[] v = new Varnode[rs.length];
        for (int i = 0; i < rs.length; i++) { Register r = currentProgram.getRegister(rs[i]); v[i] = new Varnode(r.getAddress(), r.getMinimumByteSize()); }
        return new VariableStorage(currentProgram, v);
    }
    public void run() throws Exception {
        BufferedReader br = new BufferedReader(new FileReader(getScriptArgs()[0])); String line; int n = 0;
        while ((line = br.readLine()) != null) {
            line = line.trim(); if (line.isEmpty() || line.startsWith("#")) continue;
            String[] t = line.split("\\s+");
            String[] so = t[0].split(":");
            Address a = currentProgram.getAddressFactory().getAddress(t[0]);
            Function f = getFunctionAt(a);
            if (f == null) { println("no function at " + t[0]); continue; }
            f.setName(t[1], SourceType.USER_DEFINED);
            boolean custom = false; String conv = null; String retS = null; List<String> ps = new ArrayList<>(); int purge = -1; boolean noret = false;
            for (int i = 2; i < t.length; i++) {
                if (t[i].startsWith("ret=")) retS = t[i].substring(4);
                else if (t[i].startsWith("purge=")) purge = Integer.parseInt(t[i].substring(6));
                else if (t[i].equals("noreturn")) noret = true;
                else if (t[i].equals("near") ) conv = "__cdecl16near";
                else if (t[i].equals("far")) conv = "__cdecl16far";
                else ps.add(t[i]);
            }
            for (String p : ps) if (p.contains("@")) custom = true;
            if (retS != null && retS.contains("@")) custom = true;
            if (retS == null && ps.isEmpty()) { n++; continue; }
            List<Variable> params = new ArrayList<>(); int idx = 0;
            for (String p : ps) {
                String[] nv = p.split("=", 2); String name = nv.length == 2 ? nv[0] : ("p" + (++idx)); String ty = nv.length == 2 ? nv[1] : nv[0];
                String[] ts = ty.split("@"); DataType d = dt(ts[0]);
                params.add(ts.length == 2 ? new ParameterImpl(name, d, storage(ts[1], d.getLength()), currentProgram) : new ParameterImpl(name, d, currentProgram));
            }
            Variable ret = null;
            if (retS != null) { String[] ts = retS.split("@"); DataType d = dt(ts[0]); ret = ts.length == 2 ? new ReturnParameterImpl(d, storage(ts[1], d.getLength()), currentProgram) : new ReturnParameterImpl(d, currentProgram); }
            else ret = new ReturnParameterImpl(VoidDataType.dataType, currentProgram);
            if (conv == null) conv = f.getCallingConventionName();
            if (conv == null || conv.equals("unknown")) conv = "__cdecl16far";
            f.updateFunction(custom ? null : conv, ret, params, custom ? Function.FunctionUpdateType.CUSTOM_STORAGE : Function.FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS, true, SourceType.USER_DEFINED);
            if (purge >= 0) f.setStackPurgeSize(purge);
            if (noret) f.setNoReturn(true);
            n++;
        }
        br.close(); println("applied " + n + " signatures");
    }
}
