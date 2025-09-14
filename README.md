
Description <br>
This assignment will familiarize you with the FAT32 file system. You will become familiar with file allocation tables, endieness, as well as file access. You will implement a user space shell application that is capable of interpreting a FAT32 file system image. The utility must not corrupt the file system image and should be robust. No existing kernel code or any other FAT 32 utility code may be used in your program. The code you submit for this assignment will be verified against a database consisting of github code, stackoverflow, previous student’s submissions and other internet resources. Code that is not 100% your own code will result in a grade of 0 and referral to the Office of Student Conduct.

Your program shall be named mfs.c and shall be implemented in C or C++. You shall not use the system calls system(), fork(), or any of the exec family of system calls. You may not mount the disk image.

Your program shall print out a prompt of mfs> when it is ready to accept input.

After each command completes, your program shall print the mfs> prompt and accept another line of input.

Your program will exit with status zero if the command is “quit” or “exit”.

If the user types a blank line, your program will, quietly and with no other output, print another prompt and accept a new line of input.

Your application shall be case insensitive.

The following commands shall be supported:

open
open <filename>  
This command shall open a fat32 image. Filenames of fat32 images shall not contain spaces and shall be limited to 100 characters. If the file is not found your program shall output: “Error: File system image not found.”. If a file system is already opened then your program shall output: “Error: File system image already open.”.

close
close
This command shall close the fat32 image. If the file system is not currently open your program shall output: “Error: File system not open.” Any command issued after a close, except for open, shall result in “Error: File system image must be opened first.”

save
save
This command shall write the memory resident fat32 image to the current working directory. It will use the filename of the existing open image. If the file system is not currently open your program shall output: “Error: File system not open.”

save <new filename>
This command shall write the memory resident fat32 image to the current working directory. It will use the new filename of the existing open image. If the file system is not currently open your program shall output: “Error: File system not open.” The orignal disk image shall not be modified.

quit
quit   
This command shall exit the program.

exit
exit 
This command shall exit the program.

info
info
This command shall print out information about the file system in both hexadecimal and base 10:

BPB_BytesPerSec 
BPB_SecPerClus 
BPB_RsvdSecCnt
BPB_NumFATS
BPB_FATSz32
BPB_ExtFlags
BPB_RootClus
BPB_FSInfo
stat
stat <filename> or <directory name>
This command shall print the attributes and starting cluster number of the file or directory name. If the parameter is a directory name then the size shall be 0. If the file or directory does not exist then your program shall output “Error: File not found”.

get
get <filename>
This command shall retrieve the file from the FAT 32 image and place it in your current working directory. If the file or directory does not exist then your program shall output “Error: File not found”.

get <filename> <new filename>
This command shall retrieve the file from the FAT 32 image and place it in your current working directory with the new filename. If the file or directory does not exist then your program shall output “Error: File not found”.

put
put <filename> 
This command shall read a file from the current working directory and place it in the memory resident FAT 32 image. If the file or directory does not exist then your program shall output “Error: File not found”.

put <filename> <new filename>
This command shall read a file from the current working directory and place it in the memory resident FAT 32 image using the new filename for the file. If the file or directory does not exist then your program shall output “Error: File not found”.

cd
cd <directory>
This command shall change the current working directory to the given directory. Your program shall support relative paths, e.g cd ../name and absolute paths.

ls
ls
Lists the directory contents. Your program shall support listing “.” and “..” . Your program shall not list deleted files or system volume names.

read
read <filename> <position> <number of bytes> <OPTION>
Reads from the given file at the position, in bytes, specified by the position parameter and output the number of bytes specified. The values shall be printed as hexadecimal integers in the form 0xXX by default.

OPTION FLAGS The following are flags that can change the format of the output. The user is not required to enter one.

-ascii
Print the bytes as ASCII characters

-dec
Print the bytes as decimal integers

del
del <filename>
Deletes the file from the file system

undel
undel <filename>
Un-deletes the file from the file system
