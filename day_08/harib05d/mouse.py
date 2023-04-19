if __name__ == "__main__": 
    arr1 = [
        "**************..",
	"*OOOOOOOOOOO*...",
    	"*OOOOOOOOOO*....",
    	"*OOOOOOOOO*.....",
    	"*OOOOOOOO*......",
    	"*OOOOOOO*.......",
    	"*OOOOOOO*.......",
    	"*OOOOOOOO*......",
    	"*OOOO**OOO*.....",
    	"*OOO*..*OOO*....",
    	"*OO*....*OOO*...",
    	"*O*......*OOO*..",
    	"**........*OOO*.",
    	"*..........*OOO*",
    	"............*OO*",
    	".............***"
    ]
    
    arr2 = [
        "* * * * . . . . . . . . . . . . ", 
        "* O O O * * * . . . . . . . . . ", 
        "* O O O O O O * * * . . . . . . ",
        "* O O O O O O O O O * * * . . . ",
        ". * O O O O O O O O O O O * * * ",
        ". * O O O O O O O O O O O O O O ",
        ". * O O O O O O O O O O * * * * ",
        ". . * O O O O O O * * * . . . . ",
        ". . * O O O O O O O * . . . . . ",
        ". . * O O O O * O O O * . . . . ",
        ". . . * O O O * * O O O * . . . ",
        ". . . * O O O * . * O O O * . . ",
        ". . . * O O * . . . * O O O * . ",
        ". . . . * O * . . . . * O O O * ",
        ". . . . * O * . . . . . * O O * ",
        ". . . . * O * . . . . . . * * * ",
    ]

    for i in range(0, len(arr1)): 
        s = ""
        for j in range(0, len(arr1[i])): 
            s += arr1[i][j: j + 1] + " "
        print(s)

    for i in range(0, len(arr2)): 
        s = ""
        for j in range(0, len(arr2[i])): 
            if arr2[i][j: j + 1] == " ": 
                continue
            s += arr2[i][j: j + 1]
        print("        \"" + s + "\", ")

