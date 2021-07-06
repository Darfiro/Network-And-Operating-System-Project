package logic;

import java.util.ArrayList;
import java.util.List;

public class MemInfo
{
    private long total, left;
    List<Process> topFive;

    public MemInfo(long total, long left)
    {
        this.total = total;
        this.left = left;
        topFive = new ArrayList<>();
    }

    public void addProcess(Process pr)
    {
        topFive.add(pr);
    }

    public long getTotal() { return this.total; }
    public long getLeft() { return this.left; }
    public List<Process> getProcesses() { return this.topFive; }
}
