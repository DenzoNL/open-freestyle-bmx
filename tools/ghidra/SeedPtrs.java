// Create functions at .text addresses referenced by dwords in non-.text blocks, when target follows padding/ret
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;

public class SeedPtrs extends GhidraScript {
    @Override
    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        MemoryBlock text = mem.getBlock(".text");
        Listing L = currentProgram.getListing();
        int created = 0, seen = 0;
        for (MemoryBlock b : mem.getBlocks()) {
            if (!b.isInitialized()) continue; int step = (b == text) ? 1 : 4;
            for (Address a = b.getStart(); a.compareTo(b.getEnd().subtract(3)) < 0; a = a.add(step)) {
                long v = mem.getInt(a) & 0xffffffffL;
                if (v < text.getStart().getOffset() || v > text.getEnd().getOffset()) continue;
                Address t = toAddr(v);
                if (getFunctionAt(t) != null) continue;
                byte p = getByte(t.subtract(1));
                if (!(p == (byte)0xCC || p == (byte)0x90 || p == (byte)0xC3)) continue;
                if (L.getInstructionContaining(t) != null && L.getInstructionAt(t) == null) continue;
                seen++;
                Data d = L.getDefinedDataContaining(t);
                if (d != null) L.clearCodeUnits(d.getMinAddress(), d.getMaxAddress(), false);
                new DisassembleCommand(t, null, true).applyTo(currentProgram, monitor);
                if (L.getInstructionAt(t) != null && new CreateFunctionCmd(t).applyTo(currentProgram, monitor)) created++;
            }
        }
        analyzeChanges(currentProgram);
        println("SeedPtrs candidates " + seen + " created " + created);
    }
}
