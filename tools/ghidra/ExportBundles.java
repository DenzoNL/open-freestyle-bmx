// Export one JSON bundle per function in [lo, hi): pseudo-C, disassembly, call graph, strings, globals.
// Usage: -postScript ExportBundles.java <outdir> <lo hex> <hi hex>
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class ExportBundles extends GhidraScript {
    static String q(String s) {
        StringBuilder b = new StringBuilder("\"");
        for (char c : s.toCharArray()) {
            switch (c) {
                case '"': b.append("\\\""); break;
                case '\\': b.append("\\\\"); break;
                case '\n': b.append("\\n"); break;
                case '\r': break;
                case '\t': b.append("\\t"); break;
                default:
                    if (c < 0x20 || c > 0x7e) b.append(String.format("\\u%04x", (int) c));
                    else b.append(c);
            }
        }
        return b.append('"').toString();
    }

    static String arr(Collection<String> xs) { return "[" + String.join(",", xs) + "]"; }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Path out = Paths.get(args[0]);
        Files.createDirectories(out);
        long lo = Long.parseLong(args[1], 16), hi = Long.parseLong(args[2], 16);
        DecompInterface di = new DecompInterface();
        di.toggleCCode(true);
        di.openProgram(currentProgram);
        Listing L = currentProgram.getListing();
        ReferenceManager rm = currentProgram.getReferenceManager();
        int n = 0;
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            long a = f.getEntryPoint().getOffset();
            // Skip only the incremental-link table stubs; other thunks (tail-call wrappers) are real functions.
            if (a < lo || a >= hi || (f.isThunk() && a < 0x403c30L)) continue;
            Set<String> strs = new LinkedHashSet<>(), globals = new TreeSet<>();
            StringBuilder dis = new StringBuilder();
            for (Instruction ins : L.getInstructions(f.getBody(), true)) {
                dis.append(String.format("%08x  %s\n", ins.getAddress().getOffset(), ins));
                for (Reference r : rm.getReferencesFrom(ins.getAddress())) {
                    Address t = r.getToAddress();
                    if (!t.isMemoryAddress() || r.getReferenceType().isFlow()) continue;
                    Data d = L.getDataAt(t);
                    if (d != null && d.hasStringValue()) strs.add(q(String.valueOf(d.getValue())));
                    else if (currentProgram.getMemory().getBlock(t) != null
                            && !currentProgram.getMemory().getBlock(t).isExecute())
                        globals.add(q(String.format("%08x", t.getOffset())));
                }
            }
            List<String> callers = new ArrayList<>(), callees = new ArrayList<>();
            for (Function c : f.getCallingFunctions(monitor))
                callers.add(q(String.format("%08x", c.getEntryPoint().getOffset())));
            for (Function c : f.getCalledFunctions(monitor))
                callees.add(c.isExternal() ? q("EXT:" + c.getName())
                        : q(String.format("%08x %s", c.getEntryPoint().getOffset(), c.getName())));
            String c = "";
            DecompileResults res = di.decompileFunction(f, 60, monitor);
            if (res != null && res.decompileCompleted()) c = res.getDecompiledFunction().getC();
            String json = "{\"addr\":" + q(String.format("%08x", a))
                    + ",\"name\":" + q(f.getName())
                    + ",\"size\":" + f.getBody().getNumAddresses()
                    + ",\"signature\":" + q(f.getPrototypeString(false, false))
                    + ",\"callers\":" + arr(callers) + ",\"callees\":" + arr(callees)
                    + ",\"strings\":" + arr(strs) + ",\"globals\":" + arr(globals)
                    + ",\"pseudo_c\":" + q(c) + ",\"disasm\":" + q(dis.toString()) + "}\n";
            Files.write(out.resolve(String.format("%08x.json", a)), json.getBytes());
            n++;
        }
        di.dispose();
        println("ExportBundles wrote " + n);
    }
}
