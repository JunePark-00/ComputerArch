import os

file_name = "input.txt"
f = open(file_name,"wt")
data = "+ 0x3 0x2\n- 0x1 0x2\nM R2 0x3\n+ R0 R2\n"
f.write(data)
f.close()

if os.path.exists(file_name):
    f = open(file_name,"rt")
    f_data = f.read()
    f.close()

    print(f_data)
else:
    print("Error.")

temp = f_data.split("\n")
size = len(temp)
temp2 = []

for i in range(len(temp)):
    temp2.append([''])
    temp2[i] = temp[i].split(" ")
del temp2[len(temp)-1]

R0 = 0
Rn = ""
Rn_val = 0

n1 = 0
n2 = 0

file_name2 = "output.txt"
f2 = open(file_name2,"wt")
output_data = ""
for i in range(size-1):
    move = -1
    if temp2[i][0] == "H":
        print("Complete Execution")
        break
    
    if '0x' in temp2[i][2]:
        
        if '0x' in temp2[i][1]:
            n1 = int(temp2[i][1],0)
            n2 = int(temp2[i][2],0)

        else:
            if temp2[i][1] == Rn:
                n1 = Rn_val
            elif temp2[i][1] == "R0":
                n1 = R0
        
        if temp2[i][0] == "+":
            R0 = n1 + n2
                
        elif temp2[i][0] == "-":
            R0 = n1 - n2
                
        elif temp2[i][0] == "*":
            R0 = n1 * n2

        elif temp2[i][0] == "/":
            R0 = n1 / n2
                
        if temp2[i][0] == "M":
            Rn = temp2[i][1]
            n2 = int(temp2[i][2],0)
            Rn_val = n2
            move = 1
            
    else:
        if '0x' in temp2[i][1]:
            n1 = int(temp2[i][1],0)
        else:
            if temp2[i][1] == "R0":
                n1 = R0
            elif temp2[i][1] == Rn:
                n1 = Rn_val

        if temp2[i][2] == Rn:
            n2 = Rn_val
            
            if temp2[i][0] == "+":
                R0 = n1 + n2
            elif temp2[i][0] == "-":
                R0 = n1 - n2
            elif temp2[i][0] == "*":
                R0 = n1 * n2
            elif temp2[i][0] == "/":
                R0 = n1 / n2
            elif temp2[i][0] == "M":
                if n1 == R0 and n2 == Rn_val:
                    R0 = n2
                    Rn = "R0"
                move = 1
    print("R0:",R0)
    if move == -1:
        output_data += temp2[i][0] + " " + temp2[i][1] + " " + temp2[i][2] + " >> "+ str(R0) + "\n"
    else:
        output_data += temp2[i][0] + " " + temp2[i][1] + " " + temp2[i][2] + " >> "+ Rn + ": " + str(Rn_val) + "\n"
    
f2.write(output_data)
f2.close()

if os.path.exists(file_name2):
    f2 = open(file_name2,"rt")
    f2_data = f2.read()
    f2.close()

    print(f2_data)
else:
    print("Error.")

