// Create functions at .text addresses referenced as pointers: dwords in data blocks (callback tables) and
// immediate operands of code (push offset / mov reg, offset). The target must follow padding or a ret,
// must not itself be padding, and must not lie inside an existing function (switch case labels).
// Pointers that sit in .text outside any instruction (switch jump tables) and memory operands
// (jmp [table + reg*4]) are ignored.
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.scalar.Scalar;

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
                if (getFunctionAt(t) != null || getFunctionContaining(t) != null) continue;
                if (b == text && !isImmediateOperand(L, a, v)) continue;
                byte p = getByte(t.subtract(1));
                byte first = getByte(t);
                if (!(p == (byte)0xCC || p == (byte)0x90 || p == (byte)0xC3 || (v >= RW_START && afterLeaPadding(t)))) continue;
                // Padding is not a function, but `int3; ret` (a debug-break helper) is.
                if ((first == (byte)0xCC && getByte(t.add(1)) != (byte)0xC3) || first == (byte)0x90 || first == 0) continue;
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

    static final long RW_START = 0x4a4a80L;

    /** RenderWare is aligned with `lea esi,[esi+0]` style padding behind a ret/jmp (8d 76 00, 8d bc 27 ...). */
    static final java.util.regex.Pattern AFTER_PADDING = java.util.regex.Pattern.compile(
        "(?:c3|c2[0-9a-f]{4}|e9[0-9a-f]{8}|eb[0-9a-f]{2}|cc)"
        + "(?:90|cc|8d7600|8d742600|8db42600000000|8db600000000|8dbc2700000000|8dbf00000000|8d7f00|89f6|8bf6|8d3600|8d4000)*$");

    /** True when the bytes before t are such padding behind a terminator (the previous function may not exist yet). */
    boolean afterLeaPadding(Address t) throws Exception {
        StringBuilder h = new StringBuilder();
        for (int i = 32; i >= 1; i--) h.append(String.format("%02x", getByte(t.subtract(i)) & 0xff));
        return AFTER_PADDING.matcher(h).find();
    }

    /** True when the dword at a is the immediate operand of a push/mov instruction (not a memory operand). */
    static boolean isImmediateOperand(Listing L, Address a, long v) {
        Instruction ins = L.getInstructionContaining(a);
        if (ins == null) return false;
        String m = ins.getMnemonicString();
        if (!m.equals("PUSH") && !m.equals("MOV")) return false;
        for (int i = 0; i < ins.getNumOperands(); i++) {
            for (Object o : ins.getOpObjects(i)) {
                if (o instanceof Scalar && ((Scalar) o).getUnsignedValue() == v
                        && (ins.getOperandType(i) & ghidra.program.model.lang.OperandType.SCALAR) != 0
                        && (ins.getOperandType(i) & ghidra.program.model.lang.OperandType.DYNAMIC) == 0)
                    return true;
            }
        }
        return false;
    }
}
