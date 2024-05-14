#######       ###        ########
##           ## ##          ##
##        RAS auto run      ##
##         ##     ##        ##
#######   ##       ##       ##
import os, sys, json, platform, subprocess
from datetime import datetime
 
def ParseParameters ():
    Parameter   = ""
    HelpStream  = "\n#######       ###        ########\n##           ## ##          ##\n##        RAS auto run      ##\n##         ##     ##        ##\n#######   ##       ##       ##\n\n\
[Please reference command below]\n\n\
   --c : Count PPR entry that exist in system.:  Count PPR entry in the.\n   --p : Direct print out ppr list.\n   --e : Export all ppr and ppr detail into file (PprList_%time.json).\n"
    try:
        if len (sys.argv) != 2 or len (sys.argv) == 0:
            print (HelpStream)
            sys.exit ()
        elif sys.argv[1][0]=='-' and sys.argv[1][1]=='-':
            TempVar = sys.argv[1].lower()
            if TempVar == "--h" or TempVar == "--help" or TempVar == "--?":
                print (HelpStream)
                sys.exit ()
            elif TempVar == "--c":
                Parameter  = 'c'
            elif TempVar == "--p":
                Parameter  = 'p'
            elif TempVar == "--e":
                Parameter  = 'e'
            else:
                print ("Unexpected parameter: [" + sys.argv[1] + "] input.")
                sys.exit ()
        else:
            print ("Unexpected parameter: [" + sys.argv[1] + "] input.")
            sys.exit ()
        #
        # End of parse, return to main proccess.
        #
        return Parameter
    except Exception as x:
        print ("Unexpected error occur." + x)
        sys.exit ()
 
 
########################
#####   Ap entry   #####
########################
#
# Check efivar AP have exist in system or not, if not, clone it.
#
InputVar    = ParseParameters()
try:
    subprocess.check_output(['which', 'efivar'])
except subprocess.CalledProcessError:
    print("\nWe need to install efivar tool to get variable. call dnf to do it......\n")
    try:
        subprocess.check_output(['dnf', 'install', '-y', 'efivar'])
        print("\nefivar has been installed.\n")
    except subprocess.CalledProcessError:
        print("\nSystem cannot use dnf install efivar.....\n")
        sys.exit()
#
# Define the variable
#
HeaderSize  = 0x3F+1
CrcSize     = 3
PprFile     = bytearray()
Contains    = []
DdrCount    = 0
PprCount    = 0
EachPprSize = 20
Index       = 0
Format      = {'PprValidEntry':0, 'Socket':0, 'MemoryController':0, 'Channel':0, 'SubChannel':0,
               'PseudoChannel':0, 'DIMM':0,   'Rank':0,             'SubRank':0, 'NibbleMask':0,
               'Bank':0,          'Row':0,    'PprStatus':0,        'ErrorType':0  }
#
# Start use linux tool gat variable.
#
for i in range(8):
    PprName = f"6a159d4f-6e6b-4523-aeb5-f7af1c444b0f-PprAddress{i}"
    if os.system(f"efivar -n {PprName} -e {PprName}.cat") != 0:
        print(f"Get {PprName} variable failed.\n")
        sys.exit()
    else:
        with open(f"{PprName}.cat", 'rb') as File:
            Data = File.read()[HeaderSize:]  # Skip header
            if Data[0] == 0 and Data[1] == 0 and Data[2] == 0:
                break
            PprFile.extend(Data[CrcSize:-4]) # Skip CRC and full flag
            DdrCount += 16
 
os.system("rm -rf *.cat")
#
# Parse variable and gen file into json format.
#
while Index < (DdrCount * EachPprSize):
    if PprFile[Index] != 1:
        Index += EachPprSize
        continue
    #
    # Status is correct, collect the data.
    #
    Contains.append(Format.copy())
    Contains[PprCount]['PprValidEntry']    = PprFile[Index+0]
    Contains[PprCount]['Socket']           = PprFile[Index+1]
    Contains[PprCount]['MemoryController'] = PprFile[Index+2]
    Contains[PprCount]['Channel']          = PprFile[Index+3]
    Contains[PprCount]['SubChannel']       = PprFile[Index+4]
    Contains[PprCount]['PseudoChannel']    = PprFile[Index+5]
    Contains[PprCount]['DIMM']             = PprFile[Index+6]
    Contains[PprCount]['Rank']             = PprFile[Index+7]
    Contains[PprCount]['SubRank']          = PprFile[Index+8]
    Contains[PprCount]['NibbleMask']       = hex(PprFile[Index+9]  + (PprFile[Index+10] << 8) + (PprFile[Index+11] << 16)+ (PprFile[Index+12] << 24))
    Contains[PprCount]['Bank']             = PprFile[Index+13]
    Contains[PprCount]['Row']              = hex(PprFile[Index+14]  + (PprFile[Index+15] << 8) + (PprFile[Index+16] << 16)+ (PprFile[Index+17] << 24))
    Contains[PprCount]['PprStatus']        = PprFile[Index+18]
    Contains[PprCount]['ErrorType']        = PprFile[Index+19]
    PprCount += 1
    Index    += EachPprSize
 
if InputVar == 'c':
    #
    # Only print out the count of PPR.
    #
    print ("\n <<< Now we have [" + str(PprCount) + "] PPR entry in the system. >>>")
elif InputVar == 'p':
    #
    # Print all ppr data direct.
    #
    Index = 0
    print ("\nPrint out all ppr entry :\n")
    while Index < PprCount:
        print ("  PPR index [" + str(Index) + "] >>")
        print ("    Socket : [" + str(Contains[Index]['Socket']) + "]")
        print ("    MemoryController : [" + str(Contains[Index]['MemoryController']) + "]")
        print ("    Channel : [" + str(Contains[Index]['Channel']) + "]")
        print ("    SubChannel : [" + str(Contains[Index]['SubChannel']) + "]")
        print ("    PseudoChannel : [" + str(Contains[Index]['PseudoChannel']) + "]")
        print ("    DIMM : [" + str(Contains[Index]['DIMM']) + "]")
        print ("    Rank : [" + str(Contains[Index]['Rank']) + "]")
        print ("    SubRank : [" + str(Contains[Index]['SubRank']) + "]")
        print ("    NibbleMask : [" + str(Contains[Index]['NibbleMask']) + "]")
        print ("    Bank : [" + str(Contains[Index]['Bank']) + "]")
        print ("    Row : [" + str(Contains[Index]['Row']) + "]")
        print ("    PprStatus : [" + str(Contains[Index]['PprStatus']) + "]")
        print ("    ErrorType : [" + ("CE" if Contains[Index]['ErrorType']==2 else "UCE") + "]")
       
        Index+=1
elif InputVar == 'e':
    PprListFile = "PprList_" + datetime.now().strftime('%m-%d_%H-%M-%S') + ".json"
    #
    # Record data into json file.
    #
    try:
        with open (PprListFile, 'w') as File:
            json.dump (Contains, File, ensure_ascii='utf-8', indent=2)
        print ("\n==== Create Ppr list file [" + PprListFile + "] successful!! ====")
    except:
        print ("\n==== Create Ppr list file [" + PprListFile + "] failed ====")
print ("\n====================== End of PPR tool ======================")