import sys

def main():
    filename = sys.argv[1];

    with open(filename, "wr") as file:
        line = file.readline();
        print(line);