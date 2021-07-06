package logic;

public class Process
{
    private final long pid;
    private final String comm;
    private double percentMem;

    Process(final long pid, final String comm, final double percentMem)
    {
        this.pid = pid;
        this.comm = comm;
        this.percentMem = percentMem;
    }

    public long getPid() {
        return pid;
    }

    public String getComm() {
        return comm;
    }

    public double getPercentMem() {
        return percentMem;
    }
}
