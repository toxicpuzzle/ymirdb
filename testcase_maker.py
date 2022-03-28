from asyncore import write
import math
from random import random
import sys

entries : int = 40000
max_append : int = 100

def entries_1000():
    filename = sys.argv[1];
    print(filename);

    with open(filename, "w") as file:
        for i in range(entries):
            file.write("SET " + "a" + str(i) + " " + str(i) + "\n");
        
        for entry_id in range(entries):
            entries_linked = 0

            # Append all entries after current entry to itself
            starting_id = entry_id+1
            while entries_linked < entries - entry_id:
                file.write("APPEND " + "a" + str(entry_id))
                finished_linking = False
                for i in range(max_append):
                    if (entries_linked + starting_id >= entries):
                        entries_linked += 1
                        finished_linking = True
                        break;
                    file.write(" a" + str(starting_id+entries_linked));
                    entries_linked += 1
                file.write("\n");

                if finished_linking == True:
                    break

        # file.write("LIST ENTRIES\n");
        # file.write("SUM a0\n");
        # file.write("LEN a0\n");
        file.write("FORWARD a0\n");
        file.write("BACKWARd a" + str(entries-1) + "\n");
        file.write("MAX a0\n");
        file.write("MIN a0\n");
        file.write("SNAPSHOT\n")
        file.write("PURGE a0\n")
        file.write("bye\n")

def main():
    filename = sys.argv[1];
    print(filename);

    with open(filename, "w") as file:
        for i in range(entries):
            file.write("SET " + "a" + str(i) + " " + str(i) + "\n");
        
        for entry_id in range(entries-1):
            # Append each entry to the entry next to it
            file.write("APPEND " + "a" + str(entry_id) + " " + "a" + str(entry_id+1) + "\n");

        # file.write("LIST ENTRIES\n");
        # file.write("SUM a0\n");
        # file.write("LEN a0\n");
        file.write("FORWARD a0\n");
        file.write("BACKWARd a" + str(entries-1) + "\n");
        file.write("SUM a0\n");
        file.write("MIN a0\n");
        file.write("MAX a0\n");
        file.write("MIN a0\n");
        file.write("SNAPSHOT\n")
        file.write("PURGE a0\n")
        file.write("bye\n")



main()