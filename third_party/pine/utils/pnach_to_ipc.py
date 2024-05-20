import sys

f = open(sys.argv[1])

global patch
closures=[]
closures_timer=[]

def write8():
    return "ipc->Write<uint8_t>(0x" + patch[1:8] + ", 0x" + patch[14:16] + ");"
 
def write16():
    return "ipc->Write<uint16_t>(0x" + patch[1:8] + ", 0x" + patch[12:16] + ");"
 
def write32():
    return "ipc->Write<uint32_t>(0x" + patch[1:8] + ", 0x" + patch[8:16] + ");"

def increment():
    # inc 8bit
    if(int(patch[2:3], 16) == 0):
        return "ipc->Write<uint8_t>(0x" + patch[9:16] + ", ipc->Read<uint8_t>(0x" + patch[9:16] + ") + 0x" + patch[6:8] + ");"
    # dec 8bit
    if(int(patch[2:3], 16) == 1):
        return "ipc->Write<uint8_t>(0x" + patch[9:16] + ", ipc->Read<uint8_t>(0x" + patch[9:16] + ") - 0x" + patch[6:8] + ");"
    # inc 16bit
    if(int(patch[2:3], 16) == 2):
        return "ipc->Write<uint16_t>(0x" + patch[9:16] + ", ipc->Read<uint8_t>(0x" + patch[9:16] + ") + 0x" + patch[4:8] + ");"
    # dec 16bit
    if(int(patch[2:3], 16) == 3):
        return "ipc->Write<uint16_t>(0x" + patch[9:16] + ", ipc->Read<uint8_t>(0x" + patch[9:16] + ") - 0x" + patch[4:8] + ");"

def condition():
    global closures
    closures.append(int(patch[2:4], 16))
    # equal 
    if(int(patch[8:9], 16) == 0):
        # 16 bit
        if(int(patch[1:2], 16) == 0):
            return "if(ipc->Read<uint16_t>(0x" + patch[9:16] + ") == 0x" + patch[6:8] + ") {"
        #8 bit
        else:
            return "if(ipc->Read<uint8_t>(0x" + patch[9:16] + ") == 0x" + patch[7:8] + ") {"
    # not equal 
    if(int(patch[8:9], 16) == 1):
        # 16 bit
        if(int(patch[1:2], 16) == 0):
            return "if(ipc->Read<uint16_t>(0x" + patch[9:16] + ") != 0x" + patch[6:8] + ") {"
        #8 bit
        else:
            return "if(ipc->Read<uint8_t>(0x" + patch[9:16] + ") == 0x" + patch[7:8] + ") {"

    # lesser 
    if(int(patch[8:9], 16) == 2):
        # 16 bit
        if(int(patch[1:2], 16) == 0):
            return "if(ipc->Read<uint16_t>(0x" + patch[9:16] + ") >= 0x" + patch[6:8] + ") {"
        #8 bit
        else:
            return "if(ipc->Read<uint8_t>(0x" + patch[9:16] + ") >= 0x" + patch[7:8] + ") {"

    # greater 
    if(int(patch[8:9], 16) == 3):
        # 16 bit
        if(int(patch[1:2], 16) == 0):
            return "if(ipc->Read<uint16_t>(0x" + patch[9:16] + ") <= 0x" + patch[6:8] + ") {"
        #8 bit
        else:
            return "if(ipc->Read<uint8_t>(0x" + patch[9:16] + ") <= 0x" + patch[7:8] + ") {"

# this is part of the PR "PNACH Improvements". To let you know how "good" this
# PR is, this script was made to entirely migrate from it.
def timer():
    global closures_timer
    closures_timer.append(int(patch[1:3], 16))
    return "//TIMER: topaz i hate you 0x" + patch[3:8] + " { "

def stub():
    return patch
 
switcher = {
        0: write8,
        1: write16,
        2: write32,
        3: increment,

        # -- unused in xaddgx pnach --
        4: stub,
        5: stub,
        6: stub,
        7: stub,
        8: stub,
        9: stub,
        0xa: stub,
        0xc: stub,
        0xd: stub,
        # -- unused in xaddgx pnach --


        0xb: timer,
        0xe: condition 
    }
print("//TODO: fix my indentation\nvoid pnach_thread(PCSX2Ipc *ipc)"
        + "{\nwhile(true) {\n" 
        + "\ntry {\nmsleep(16); // every frame at 60fps\n")
for i in f.readlines():
    i=i.replace("--//", "//--").replace("\n","").replace("\t"," ")
    if "patch=1,EE," in i[:11]:
        patch= i[11:].replace(",extended,", "")
        if(len(i.split(" ", 1)[0]) != 37):
            patch = patch[0:8] + "0000" + patch[8:12]
        func = switcher.get(int(patch[0], 16), "nothing")
        eol=""
        for y in range(0, len(closures)):
            closures[y]=closures[y]-1
            if(closures[y]==0):
                eol+=" }"
        for y in range(0, len(closures_timer)):
            closures_timer[y]=closures_timer[y]-1
            if(closures_timer[y]==0):
                eol+="// }"
        print(func() + eol)
    else:
        if not "gametitle" in i:
            print(i)
print("}\ncatch(...) {\n}\n//do nothing on failure\n}\n}")
