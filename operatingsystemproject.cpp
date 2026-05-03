#include<iostream>
#include<fstream>
#include<string>
#include<unordered_map>
#include<vector>
#include<queue>
using namespace std;
struct SystemConfiguration {
    int ramSize;
    int pageSize;
    int tlbSize;
    int tlbLatency;
    int ramLatency;
    int diskReadLatency;
    int diskWriteLatency;
    string replacementPolicy;
    int numFrames;
    int offsetBits;
};
class configurationParser {
public:
    SystemConfiguration loadConfig(string filename) {
        SystemConfiguration config;
        config.ramSize = 0;
        config.pageSize = 0;
        config.tlbSize = 0;
        config.tlbLatency = 0;
        config.ramLatency = 0;
        config.diskReadLatency = 0;
        config.diskWriteLatency = 0;
        config.replacementPolicy = "";
        config.numFrames = 0;
        config.offsetBits = 0;

        ifstream file(filename);

        if (!file.is_open()) {
            cout << "File does not exist" << endl;
            exit(0);
        }

        string key;
        char equalSign;

        while (file >> key) {
            if (key == "RAM_SIZE") {
                file >> equalSign >> config.ramSize;
            }
            else if (key == "PAGE_SIZE") {
                file >> equalSign >> config.pageSize;
            }
            else if (key == "TLB_SIZE") {
                file >> equalSign >> config.tlbSize;
            }
            else if (key == "TLB_LATENCY") {
                file >> equalSign >> config.tlbLatency;
            }
            else if (key == "RAM_LATENCY") {
                file >> equalSign >> config.ramLatency;
            }
            else if (key == "DISK_READ_LATENCY") {
                file >> equalSign >> config.diskReadLatency;
            }
            else if (key == "DISK_WRITE_LATENCY") {
                file >> equalSign >> config.diskWriteLatency;
            }
            else if (key == "REPLACEMENT_POLICY") {
                file >> equalSign >> config.replacementPolicy;
            }
        }

        validateConfig(config);
        calculateDerivedValues(config);
        return config;
    }

    void printConfig(SystemConfiguration config) {
        cout << "         CONFIGURATION    " << endl;
        cout << "RAM Size           : " << config.ramSize << " bytes" << endl;
        cout << "Page Size          : " << config.pageSize << " bytes" << endl;
        cout << "TLB Size           : " << config.tlbSize << " entries" << endl;
        cout << "TLB Latency        : " << config.tlbLatency << " ns" << endl;
        cout << "RAM Latency        : " << config.ramLatency << " ns" << endl;
        cout << "Disk Read Latency  : " << config.diskReadLatency << " ns" << endl;
        cout << "Disk Write Latency : " << config.diskWriteLatency << " ns" << endl;
        cout << "Replacement Policy : " << config.replacementPolicy << endl;
        cout << "Number of Frames   : " << config.numFrames << endl;
        cout << "Offset Bits        : " << config.offsetBits << endl;
    }

    bool isPowerOfTwo(int n) {
        if (n <= 0) {
            return false;
        }

        while (n % 2 == 0) {
            n = n / 2;
        }

        return n == 1;
    }

    int calculateOffsetBits(int pageSize) {
        int bits = 0;

        while (pageSize > 1) {
            pageSize = pageSize / 2;
            bits++;
        }

        return bits;
    }

    void validateConfig(SystemConfiguration config) {
        if (config.ramSize <= 0) {
            cout << "Error: RAM_SIZE must be greater than 0." << endl;
            exit(0);
        }

        if (config.pageSize <= 0) {
            cout << "Error: PAGE_SIZE must be greater than 0." << endl;
            exit(0);
        }

        if (config.tlbSize <= 0) {
            cout << "Error: TLB_SIZE must be greater than 0." << endl;
            exit(0);
        }

        if (config.ramSize % config.pageSize != 0) {
            cout << "Error: RAM_SIZE must be divisible by PAGE_SIZE." << endl;
            exit(0);
        }

        if (!isPowerOfTwo(config.pageSize)) {
            cout << "Error: PAGE_SIZE must be a power of 2." << endl;
            exit(0);
        }

        if (config.replacementPolicy != "FIFO" &&
            config.replacementPolicy != "LRU" &&
            config.replacementPolicy != "OPT") {
            cout << "Error: REPLACEMENT_POLICY must be FIFO, LRU, or OPT." << endl;
            exit(0);
        }
    }

    void calculateDerivedValues(SystemConfiguration &config) {
        config.numFrames = config.ramSize / config.pageSize;
        config.offsetBits = calculateOffsetBits(config.pageSize);
    }
};
struct MemoryAccess {
    char operation;       //R OR W
    unsigned int virtualAddress;
};
struct AddressParts {
    unsigned int vpn;
    unsigned int offset;
};
struct PageTableEntry {
    bool valid;
    bool dirty;
    int frameNumber;
};
struct TLBEntry {
    bool valid;
    unsigned int vpn;
    int frameNumber;
};
class TraceParser {
public:
    MemoryAccess readOneAccess(ifstream &file) {
        MemoryAccess access;
        file >> access.operation >> access.virtualAddress;
        return access;
    }

    AddressParts splitAddress(unsigned int address, int pageSize) {
        AddressParts parts;
        parts.vpn = address / pageSize;
        parts.offset = address % pageSize;
        return parts;
    }

    void printOneAccess(MemoryAccess access, AddressParts parts) {
        cout << "Operation      : " << access.operation << endl;
        cout << "Virtual Address: " << access.virtualAddress << endl;
        cout << "VPN            : " << parts.vpn << endl;
        cout << "Offset         : " << parts.offset << endl;
        cout  << endl;
    }
};
class PageTableManager {
private:
    unordered_map<unsigned int, PageTableEntry> pageTable;  //vpn=key
    vector<int> freeframes;
    queue<unsigned int> fifoQueue;  //vpn
    vector<unsigned int> lruList;   //Front of vector  = least recently used page
    //Back of vector   = most recently used page
public:
    void initialize(int numberofframes) {
        for (int i=numberofframes-1;i>=0;i--) {
            freeframes.push_back(i);
        }
    }
    bool hasfreeframes() {
        return !freeframes.empty();
    }
    int allocateFrame() {
        int index = freeframes.back();
        freeframes.pop_back(); //frame is occupied
        return index;
    }
    bool pagepresent(unsigned int vpn) {
        if (pageTable.find(vpn) != pageTable.end() && pageTable[vpn].valid) {
            return true;
        }
        return false;
    }
    int getframenumber(unsigned int vpn) {
        return pageTable[vpn].frameNumber;
    }
    //This is the function that actually inserts a page into the page table after a page fault.
    void addentryintotable(unsigned int vpn,int frameNumber,char operation) {
        PageTableEntry entry;
        entry.frameNumber = frameNumber;
        entry.valid = true;
        if (operation == 'W') {
            entry.dirty=true;
        }
        else if (operation == 'R') {
            entry.dirty=false;
        }
        pageTable[vpn] = entry;
    }
    //This function is used in this situation:
    //page is already present in RAM
    //operation is W
    //so page should become dirty
    void markdirty(unsigned int vpn) {
        if (pageTable.find(vpn) != pageTable.end()) {
            pageTable[vpn].dirty=true;
        }
    }
    void printPageTable() {
        cout<<"Page Table:"<<endl;
       unordered_map<unsigned int, PageTableEntry>::iterator it;
        for (it = pageTable.begin(); it != pageTable.end(); it++) {
            cout<<"Virtual pagenumber"<<it->first<<endl;
            cout << "  Frame: " << it->second.frameNumber<<endl;
             cout<< "  Valid: " << it->second.valid<<endl;
             cout<< "  Dirty: " << it->second.dirty << endl;
        }
        cout<<endl;
    }
    void addtoqueue(unsigned int vpn) {
        fifoQueue.push(vpn);
    }
    unsigned int getFIFOPage() { //get victim page
        return fifoQueue.front();
    }
    void removefifopage() {
        fifoQueue.pop();
    }
    //invalidate old page in page table
    void invalidatepage(unsigned int vpn) {
        if (pageTable.find(vpn) != pageTable.end()) {
            pageTable[vpn].valid=false;
        }
    }
    bool isPageDirty(unsigned int vpn) {
        if (pageTable.find(vpn) != pageTable.end()) {
            return pageTable[vpn].dirty;
        }
        return false;
    }
    void updateLRU(unsigned int vpn) {
        // First remove the VPN if it already exists in lruList
        for (int i = 0; i < lruList.size(); i++) {
            if (lruList[i] == vpn) {
                lruList.erase(lruList.begin() + i);
                break;
            }
        }

        // Add the VPN at the end because it is now most recently used
        lruList.push_back(vpn);
    }

    unsigned int getLRUPage() {
        // The first page is the least recently used page
        return lruList.front();
    }

    void removeLRUPage(unsigned int vpn) {
        for (int i = 0; i < lruList.size(); i++) {
            if (lruList[i] == vpn) {
                lruList.erase(lruList.begin() + i);
                break;
            }
        }
    }

    void printLRUList() {
        cout << "LRU List: ";

        for (int i = 0; i < lruList.size(); i++) {
            cout << lruList[i] << " ";
        }

        cout << endl;
    }
    unsigned int getOPTPage(vector<MemoryAccess> allAccesses, int currentIndex, int pageSize) {
        unsigned int victimVPN = 0;
        int farthestUse = -1;

        unordered_map<unsigned int, PageTableEntry>::iterator it;

        for (it = pageTable.begin(); it != pageTable.end(); it++) {
            unsigned int currentVPN = it->first;

            if (it->second.valid == true) {
                int nextUse = -1;

                for (int i = currentIndex + 1; i < allAccesses.size(); i++) {
                    AddressParts futureParts;
                    futureParts.vpn = allAccesses[i].virtualAddress / pageSize;
                    futureParts.offset = allAccesses[i].virtualAddress % pageSize;

                    if (futureParts.vpn == currentVPN) {
                        nextUse = i;
                        break;
                    }
                }

                if (nextUse == -1) {
                    return currentVPN;
                }

                if (nextUse > farthestUse) {
                    farthestUse = nextUse;
                    victimVPN = currentVPN;
                }
            }
        }

        return victimVPN;
    }
};
class simulationresults {
private:
    int totalnumberofaccesses;
    int pagehits;
    int pagefaults;
    int writeaccesses;
    int pagereplacements;
    int tlbhits;
    int tlbmisses;
    long long totalsimulatedtime;
    double eat;
public:
    simulationresults() {
        totalnumberofaccesses=0;
        pagehits=0;
        pagefaults=0;
        writeaccesses=0;
        pagereplacements=0;
        tlbhits = 0;
        tlbmisses = 0;
        totalsimulatedtime = 0;
        eat = 0.0;

    }
    void incrementpagehits() {
        pagehits++;
    }
    void incrementpagefaults() {
        pagefaults++;
    }
    void incrementwriteaccesses() {
        writeaccesses++;
    }
    void incrementpagereplacements() {
        pagereplacements++;

    }
    void incrementtotalnumberofaccesses() {
        totalnumberofaccesses++;
    }
    void incrementtlbhits() {
        tlbhits++;
    }

    void incrementtlbmisses() {
        tlbmisses++;
    }
    void addsimulatedtime(long long time) {
        totalsimulatedtime = totalsimulatedtime + time;
    }

    void calculateeat() {
        if (totalnumberofaccesses > 0) {
            eat = (double)totalsimulatedtime / totalnumberofaccesses;
        }
    }
    void displaystats() {
        cout << "Total number of accesses: " << totalnumberofaccesses << endl;
        cout << "Page hit count: " << pagehits << endl;
        cout << "Page fault count: " << pagefaults << endl;
        cout << "Write access count: " << writeaccesses << endl;
        cout << "Page replaced count: " << pagereplacements << endl;
        cout << "TLB hit count: " << tlbhits << endl;
        cout << "TLB miss count: " << tlbmisses << endl;
        cout << "Total simulated time: " << totalsimulatedtime << endl;
        cout << "Effective Access Time: " << eat << endl;
    }

};
class TLBManager {
private:
vector<TLBEntry> tlb;
    int maxSize;

public:
    TLBManager(int maxSize) {
        this->maxSize = maxSize;
    }
    bool isTLBHit(unsigned int vpn) {
        for (int i = 0; i < tlb.size(); i++) {
            if (tlb[i].valid == true && tlb[i].vpn == vpn) {
                return true;
            }
        }
        return false;
    }
    int getFrameNumberFromTLB(unsigned int vpn) {
        for (int i = 0; i < tlb.size(); i++) {
            if (tlb[i].valid == true && tlb[i].vpn == vpn) {
                return tlb[i].frameNumber;
            }
        }
        return -1;
    }
    //Inserting into TLB means caching the translation VPN → FrameNumber so future accesses to that page can skip page-table lookup
    void insertIntoTLB(unsigned int vpn, int frameNumber) {
        TLBEntry entry;
        entry.valid = true;
        entry.vpn = vpn;
        entry.frameNumber = frameNumber;

        if (tlb.size() < maxSize) {
            tlb.push_back(entry);
        }
        else {
            tlb.erase(tlb.begin());  // This is a simple FIFO-style replacement for TLB.
            // The oldest inserted TLB entry is removed first.
            tlb.push_back(entry);
        }
    }
    void invalidateTLBEntry(unsigned int vpn) {
        for (int i = 0; i < tlb.size(); i++) {
            if (tlb[i].valid == true && tlb[i].vpn == vpn) {
                tlb[i].valid = false;
            }
        }
    }
    void printTLB() {
        cout << "TLB Entries:" << endl;

        for (int i = 0; i < tlb.size(); i++) {
            cout << "Entry " << i << endl;
            cout << "  Valid: " << tlb[i].valid << endl;
            cout << "  VPN: " << tlb[i].vpn << endl;
            cout << "  Frame: " << tlb[i].frameNumber << endl;
        }

        cout << endl;
    }
};
class Simulator {
private:
    SystemConfiguration config;
    TraceParser traceParser;
    PageTableManager memoryManager;
    TLBManager tlbManager;
    simulationresults stats;
public:
    Simulator(SystemConfiguration cfg)
    : config(cfg), tlbManager(cfg.tlbSize) {
        memoryManager.initialize(config.numFrames);
    }
    void runSimulation(string traceFileName) {
        ifstream traceFile(traceFileName);

        if (!traceFile.is_open()) {
            cout << "Trace file does not exist" << endl;
            return;
        }

        vector<MemoryAccess> allAccesses;

        while (!traceFile.eof()) {
            traceFile >> ws;

            if (traceFile.eof()) {
                break;
            }

            MemoryAccess access = traceParser.readOneAccess(traceFile);

            if (access.operation != 'R' && access.operation != 'W') {
                cout << "Invalid operation in trace file" << endl;
                return;
            }

            allAccesses.push_back(access);
        }

        traceFile.close();
        for (int currentIndex = 0; currentIndex < allAccesses.size(); currentIndex++) {
            MemoryAccess access = allAccesses[currentIndex];

            stats.incrementtotalnumberofaccesses();

            if (access.operation == 'W') {
                stats.incrementwriteaccesses();
            }

            AddressParts parts = traceParser.splitAddress(access.virtualAddress, config.pageSize);

            cout << "   Operation      : " << access.operation << endl;
            cout << "Virtual Address: " << access.virtualAddress << endl;
            cout << "VPN            : " << parts.vpn << endl;
            cout << "Offset         : " << parts.offset << endl;

            if (tlbManager.isTLBHit(parts.vpn)) {
    stats.incrementtlbhits();

    int frame = tlbManager.getFrameNumberFromTLB(parts.vpn);
    stats.addsimulatedtime(config.tlbLatency + config.ramLatency);

    cout << "TLB Status     : Hit" << endl;
    cout << "Frame Number   : " << frame << endl;

    memoryManager.updateLRU(parts.vpn);

    if (access.operation == 'W') {
        memoryManager.markdirty(parts.vpn);
    }
}
else {
    stats.incrementtlbmisses();
    cout << "TLB Status     : Miss" << endl;

    if (memoryManager.pagepresent(parts.vpn)) {
        stats.incrementpagehits();

        int frame = memoryManager.getframenumber(parts.vpn);
        stats.addsimulatedtime(config.tlbLatency + config.ramLatency + config.ramLatency);

        cout << "Page Status    : Page Hit" << endl;
        cout << "Frame Number   : " << frame << endl;

        memoryManager.updateLRU(parts.vpn);
        tlbManager.insertIntoTLB(parts.vpn, frame);

        if (access.operation == 'W') {
            memoryManager.markdirty(parts.vpn);
        }
    }
    else {
        stats.incrementpagefaults();
        cout << "Page Status    : Page Fault" << endl;

        if (memoryManager.hasfreeframes()) {
            int frame = memoryManager.allocateFrame();

            stats.addsimulatedtime(config.tlbLatency + config.ramLatency + config.diskReadLatency + config.ramLatency);

            memoryManager.addentryintotable(parts.vpn, frame, access.operation);
            memoryManager.addtoqueue(parts.vpn);
            memoryManager.updateLRU(parts.vpn);
            tlbManager.insertIntoTLB(parts.vpn, frame);

            cout << "Loaded into Frame: " << frame << endl;
        }
        else {
            stats.incrementpagereplacements();
            unsigned int victimVPN=0;

            if (config.replacementPolicy == "FIFO") {
                victimVPN = memoryManager.getFIFOPage();
                cout << "RAM Full       : FIFO Replacement Needed" << endl;
            }
            else if (config.replacementPolicy == "LRU") {
                victimVPN = memoryManager.getLRUPage();
                cout << "RAM Full       : LRU Replacement Needed" << endl;
            }
            else if (config.replacementPolicy == "OPT") {
                victimVPN = memoryManager.getOPTPage(allAccesses, currentIndex, config.pageSize);
                cout << "RAM Full       : OPT Replacement Needed" << endl;
            }

            int victimFrame = memoryManager.getframenumber(victimVPN);

            cout << "Victim VPN     : " << victimVPN << endl;
            cout << "Victim Frame   : " << victimFrame << endl;

            if (memoryManager.isPageDirty(victimVPN)) {
                cout << "Victim Dirty   : Yes" << endl;
                stats.addsimulatedtime(config.tlbLatency + config.ramLatency + config.diskWriteLatency + config.diskReadLatency + config.ramLatency);
            }
            else {
                cout << "Victim Dirty   : No" << endl;
                stats.addsimulatedtime(config.tlbLatency + config.ramLatency + config.diskReadLatency + config.ramLatency);
            }

            memoryManager.invalidatepage(victimVPN);
            tlbManager.invalidateTLBEntry(victimVPN);

            if (config.replacementPolicy == "FIFO") {
                memoryManager.removefifopage();
            }
            else if (config.replacementPolicy == "LRU") {
                memoryManager.removeLRUPage(victimVPN);
            }
            else if (config.replacementPolicy == "OPT") {
                memoryManager.removeLRUPage(victimVPN);
            }

            memoryManager.addentryintotable(parts.vpn, victimFrame, access.operation);
            memoryManager.addtoqueue(parts.vpn);
            memoryManager.updateLRU(parts.vpn);
            tlbManager.insertIntoTLB(parts.vpn, victimFrame);

            cout << "New Page Loaded into Frame: " << victimFrame << endl;
        }
    }
}

tlbManager.printTLB();
memoryManager.printLRUList();
cout << "=============================" << endl;
        }
        stats.calculateeat();
        cout << endl;
        memoryManager.printPageTable();
        cout << endl;
        stats.displaystats();
    }
};
int main() {
    configurationParser parser;
    SystemConfiguration config = parser.loadConfig("config.txt");
    parser.printConfig(config);

    Simulator sim(config);
    sim.runSimulation("trace.txt");

    return 0;
}
