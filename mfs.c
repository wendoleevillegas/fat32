#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <dirent.h>
#include <stdint.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 32    
#define MAX_FILE_NAME 100 

char error_message[] = "Error. File not found.\n";

FILE *fd;
char file[MAX_FILE_NAME];

int LBAToOffset(int32_t sector);
int rootsize(FILE *fd);
int16_t NextLB(uint32_t sector);
void compare(char *filename);

struct __attribute__((__packed__)) DirectoryEntry
{
	char DIR_Name[11];
	uint8_t DIR_Attr;
	uint8_t Unused1[8];
	uint16_t DIR_FirstClusterHigh;
	uint8_t Unused2[4];
	uint8_t DIR_FirstClusterLow;
	uint32_t DIR_FileSize;
};
struct DirectoryEntry dir[16]; // 16 per cluster

struct BPB
{
  char      BS_OEMName[8];
  int16_t   BPB_BytesPerSec;
  int8_t    BPB_SecPerClus;
  int16_t   BPB_RsvdSecCnt;
  int8_t    BPB_NumFATs;
  int16_t   BPB_RootEntCnt;
  char      BS_Vollab[11];
  int32_t   BPB_FATSz32;
  int32_t   BPB_RootClus;

  //int32_t   RootDirSectors = 0;
  //int32_t   FirsDataSector = 0;
  //int32_t   FirstSectorofCluster = 0;
};
struct BPB bpb;


int LBAToOffset(int32_t sector)
{
  // added the declaration and gathering of values, had to change the definition a bit
  int16_t BPB_BytesPerSec;
  int16_t BPB_RsvdSecCnt;
  int8_t BPB_NumFATs;
  int32_t BPB_FATSz32;

  fseek(fd, 11, SEEK_SET);
  fread(&BPB_BytesPerSec, 2, 1, fd);

  fseek(fd, 14, SEEK_SET);
  fread(&BPB_RsvdSecCnt, 2, 1, fd);

  fseek(fd, 16, SEEK_SET);
  fread(&BPB_NumFATs, 1, 1, fd);

  fseek(fd, 36, SEEK_SET);
  fread(&BPB_FATSz32, 4, 1, fd);

	return (( sector - 2 ) * BPB_BytesPerSec) + 
	(BPB_BytesPerSec * BPB_RsvdSecCnt) +
	(BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec);
}


int16_t NextLB(uint32_t sector)
{
	uint32_t FATAddress = (bpb.BPB_BytesPerSec * bpb.BPB_RsvdSecCnt) + (sector * 4);
	int16_t val;
	fseek(fd, FATAddress, SEEK_SET);
	fread(&val, 2, 1, fd);
	return val;
}

// made a function that would calculate the root directory for me
int root_size(FILE *fd)
{
      uint16_t BPB_BytsPerSec;
      uint32_t BPB_FATSz32;
      uint8_t BPB_NumFATs;
      uint16_t BPB_RsvdSecCnt;
      int TOTAL_FAT_SIZE;

      fd = fopen("fat32.img", "r");

      // TOTAL FAT SIZE aka ROOT DIRECTORY:
      // (BPB_NumFATS * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec)

      // NumFATS
      fseek(fd, 16, SEEK_SET);
      fread(&BPB_NumFATs, 1, 1, fd);

      // FATSz32
      fseek(fd, 36, SEEK_SET);
      fread(&BPB_FATSz32, 4, 1, fd);

      // BytsPerSec
      fseek(fd, 11, SEEK_SET);
      fread(&BPB_BytsPerSec, 2, 1, fd);

      //RsvdSecCnt
      fseek(fd, 14, SEEK_SET);
      fread(&BPB_RsvdSecCnt, 2, 1, fd);

      TOTAL_FAT_SIZE = ((BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) +
      (BPB_RsvdSecCnt * BPB_BytsPerSec));

      return TOTAL_FAT_SIZE;
}

// compare.c, had to change it so I wouldn't have to hardcode filename
void compare(char *filename)
{
  char expanded_name[12];
  memset(expanded_name, ' ', 12);

  char *token = strtok(filename, ".");

  if(token)
  {
    strncpy(expanded_name, token, strlen(token));
    token = strtok(NULL, ".");

    if(token)
    {
      strncpy((char*)(expanded_name+8), token, strlen(token));
    }

    expanded_name[11] = '\0';

    int i;
    for(i = 0; i < 11; i++)
    {
      expanded_name[i] = toupper(expanded_name[i]);
    }
  }

  else
  {
    strncpy(expanded_name, filename, 11);
  }

  strncpy(file, expanded_name, 12);
}

int main()
{

  char *command_string = (char*)malloc( MAX_COMMAND_SIZE );

  while(1)
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandi line.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something.
    while(!fgets(command_string, MAX_COMMAND_SIZE, stdin));

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];
    int token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *argument_pointer;                                         
                                                           
    char *working_string  = strdup(command_string);                

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    
    char *head_ptr = working_string;
    
    // Tokenize the input with whitespace used as the delimiter
    while (((argument_pointer = strsep(&working_string, WHITESPACE)) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_pointer, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    if(token[0] == NULL)
      continue;

    // OPEN
    if(strcasecmp(token[0], "open") == 0 && token[1] != NULL)
    {
      uint16_t BPB_BytsPerSec;
      uint32_t BPB_FATSz32;
      uint8_t BPB_NumFATs;
      uint16_t BPB_RsvdSecCnt;
      int TOTAL_FAT_SIZE;

      if(strlen(token[1]) > 100)
      {
        printf("Error: Filename is too long. Please limit it to 100 characters.\n");
      }

      // TOTAL FAT SIZE aka ROOT DIRECTORY:
      // (BPB_NumFATS * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec)

      else if(fd)
      {
        printf("Error: File system image already opened.\n");
      }

      else
      { 
        fd = fopen(token[1], "r");


        if(fd == NULL)
        {
          printf("Error: File system image not found.\n");
        }

        else
        {
            // NumFATS
          fseek(fd, 16, SEEK_SET);
          fread(&BPB_NumFATs, 1, 1, fd);

          // FATSz32
          fseek(fd, 36, SEEK_SET);
          fread(&BPB_FATSz32, 4, 1, fd);

          // BytsPerSec
          fseek(fd, 11, SEEK_SET);
          fread(&BPB_BytsPerSec, 2, 1, fd);

          //RsvdSecCnt
          fseek(fd, 14, SEEK_SET);
          fread(&BPB_RsvdSecCnt, 2, 1, fd);

          TOTAL_FAT_SIZE = ((BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) +
          (BPB_RsvdSecCnt * BPB_BytsPerSec));
          
          fseek(fd, TOTAL_FAT_SIZE, SEEK_SET);
          
          for(int i = 0; i < 16; i++)
          {
            fread(&dir[i], 32, 1, fd);
          }
        }
      }

    }

    // CLOSE
    else if(strcasecmp(token[0], "close") == 0 && token[1] == NULL)
    {
      // if file is open
      if(fd)
      {
        fclose(fd);
        fd = NULL;
      }
      
      // trying to close once it's already closed
      else
      {
        printf("Error: File system not open.\n");
      }
    }

    // trying to perform another command that's not open
    else if(fd == NULL && strcasecmp(token[0], "open") != 0)
    {
      printf("Error: File system image must be opened first.\n");
    }


    // INFO
    else if(strcasecmp(token[0], "info") == 0)
    {
      uint16_t BPB_BytesPerSec;
      uint8_t BPB_SecPerClus;
      uint16_t BPB_RsvdSecCnt;
      uint8_t BPB_NumFATS;
      uint32_t BPB_FATSz32;
      uint16_t BPB_ExtFlags;
      uint32_t BPB_RootClus;
      uint16_t BPB_FSInfo;

      fd = fopen("fat32.img", "r");

      fseek(fd, 11, SEEK_SET);
      fread(&BPB_BytesPerSec, 2, 1, fd);

      fseek(fd, 13, SEEK_SET);
      fread(&BPB_SecPerClus, 1, 1, fd);

      fseek(fd, 14, SEEK_SET);
      fread(&BPB_RsvdSecCnt, 2, 1, fd);

      fseek(fd, 16, SEEK_SET);
      fread(&BPB_NumFATS, 1, 1, fd);

      fseek(fd, 36, SEEK_SET);
      fread(&BPB_FATSz32, 4, 1, fd);

      fseek(fd, 40, SEEK_SET);
      fread(&BPB_ExtFlags, 2, 1, fd);

      fseek(fd, 44, SEEK_SET);
      fread(&BPB_RootClus, 4, 1, fd);

      fseek(fd, 48, SEEK_SET);
      fread(&BPB_FSInfo, 2, 1, fd);

      printf("BPB_BytesPerSec\t%x\t%d\n", BPB_BytesPerSec, BPB_BytesPerSec);
      printf("BPB_SecPerClus\t%x\t%d\n", BPB_SecPerClus, BPB_SecPerClus);
      printf("BPB_RsvdSecCnt\t%x\t%d\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);
      printf("BPB_NumFATS\t%x\t%d\n", BPB_NumFATS, BPB_NumFATS);
      printf("BPB_FATSz32\t%x\t%d\n", BPB_FATSz32, BPB_FATSz32); // 32-bit count of sectors occupied by ONE FAT
      printf("BPB_ExtFlags\t%x\t%d\n", BPB_ExtFlags, BPB_ExtFlags);
      printf("BPB_RootClus\t%x\t%d\n", BPB_RootClus, BPB_RootClus);
      printf("BPB_FSInfo\t%x\t%d\n", BPB_FSInfo, BPB_FSInfo);

    }

    // LS, do NOT list deleted files OR system volume names (check .txt file)
    else if(strcasecmp(token[0], "ls") == 0)
    {
      
      for(int i = 0; i < 16; i++)
      {
        struct DirectoryEntry entry = dir[i];

        // only show file if their attributes are 0x01, 0x10, or 0x20
        if(entry.DIR_Attr == 0x01 || entry.DIR_Attr == 0x10 || entry.DIR_Attr == 0x20)
        {
          printf("%.*s\n", 11, entry.DIR_Name);
        }

        // if attribtues are not 01, 10, or 20, don't print
        else
        {
          continue;
        }
      }

      //TODO: change so it is able to handle directories with more than 16 files
    }
    
    //STAT: stat <filename> or stat <directory name>
    // print attributes
    // print starting cluster of the file OR directory name
    // if parameter is a directory name, size == 0 ?
    else if(strcasecmp(token[0], "stat") == 0 && token[1] != NULL)
    {
      uint16_t BPB_BytsPerSec;
      uint16_t BPB_RsvdSecCnt;

      //BytsPerSec
      fseek(fd, 11, SEEK_SET);
      fread(&BPB_BytsPerSec, 2, 1, fd);

      //RsvdSecCnt
      fseek(fd, 14, SEEK_SET);
      fread(&BPB_RsvdSecCnt, 2, 1, fd);

      compare(token[1]);

      for(int i = 0; i < 16; i++)
      {
        // got TA help for this portion
        struct DirectoryEntry entry = dir[i];
        char name[12];
        memset(name, ' ', 12);
        memcpy(name, entry.DIR_Name, 12); 
        name[11] = '\0';

        if(strcmp(file, name) == 0)
        {
          printf("Attribute: %d\n", entry.DIR_Attr);
          printf("Starting cluster: %d\n",(BPB_RsvdSecCnt * BPB_BytsPerSec)); // unsure of what equation to use here
        }
      }
    }


   // CD
  else if(strcasecmp(token[0], "cd") == 0 && token[1] != NULL)
  {
    compare(token[1]);
    int offset;
    for(int i = 0; i < 16; i++)
    {
      struct DirectoryEntry entry = dir[i];
      char name[12];
      memset(name, ' ', 12);
      memcpy(name, entry.DIR_Name, 12); 
      name[11] = '\0';

      // if directory name is . , make no changes
      if(strcmp(file, ".") == 0)
      {
        continue;
      }

      else if(strcmp(file, "..") == 0)
      {
        // setting offset to the value of the parent's directory
        offset = LBAToOffset(2);
      }

      else 
      { 
        for( int i = 0; i < 16; i++)
        {
          if(strcmp(file, name) == 0)
          {
            // set offset to the entry's starting cluster
            offset = entry.DIR_FirstClusterLow;
          }
        }
      }

      // moving file to calculated offset of the wanted directory
      // load the entries of the current directory you are now in
      fseek(fd, offset, SEEK_SET);
      fread(&dir[i], 32, 16, fd);
    }
  }

  // READ
  else if(strcasecmp(token[0], "read") == 0 && token[1] != NULL)
    {
      char *filename = token[1];
      int position = atoi(token[2]);
      int number_of_bytes = atoi(token[3]);
      //int root_offset = root_size(fd);
      uint8_t buffer[number_of_bytes];

      compare(filename); // making sure user input and FAT32 format match

      for(int i = 0; i < 16; i++)
      {
        // read the file's info
        struct DirectoryEntry entry = dir[i];
        int cluster = entry.DIR_FirstClusterLow;

        int offset = LBAToOffset(cluster);
        //printf("offset %d\n", offset);

        fseek(fd, offset + position, SEEK_SET);
        fread(buffer, 1, number_of_bytes, fd);

        // printing the bytes requested
        for(int i = 0; i < number_of_bytes; i++)
        {
          printf("%01x ", buffer[i]);
        }
        printf("\n");
        break;
      }

    // TODO: fix to make it work between clusters, add option flags
    }

  else if(strcasecmp(token[0], "del") == 0 && token[1] != NULL)
  {
    for(int i = 0; i < 16; i++)
    {
      struct DirectoryEntry entry = dir[i];
      char name[12];
      memset(name, ' ', 12);
      memcpy(name, entry.DIR_Name, 12); 
      name[11] = '\0';

      // changing attribute of chosen file to be deleted (change attr to 0xe5)
      if(strcmp(file,name) == 0)
      {
        entry.DIR_Attr = 0xe5;
      }

    }
  }

  else if(strcasecmp(token[0], "undel") == 0 && token[1] != NULL)
  {
    for(int i = 0; i < 16; i++)
    {
      struct DirectoryEntry entry = dir[i];
      char name[12];
      memset(name, ' ', 12);
      memcpy(name, entry.DIR_Name, 12); 
      name[11] = '\0';

      // changing attribute to 0x01, 0x01 turns it back to a file
      if(strcmp(file,name) == 0)
      {
        entry.DIR_Attr = 0x01;
      }
    }
  }

    // EXIT & QUIT
    else if(strcasecmp(token[0], "exit") == 0 || strcasecmp(token[0], "quit") == 0)
    {
      exit(0);
    }

    else if (strcasecmp(token[0], "get") == 0)
    {
      uint16_t BPB_BytesPerSec;
      uint8_t BPB_SecPerClus;
      uint16_t BPB_RsvdSecCnt;
      uint8_t BPB_NumFATS;
      uint32_t BPB_FATSz32;
      uint16_t BPB_ExtFlags;
      uint32_t BPB_RootClus;
      uint16_t BPB_FSInfo;

      //  int total_fat_size = root_size(fd);

      fd = fopen("fat32.img", "r");

      fseek(fd, 11, SEEK_SET);
      fread(&BPB_BytesPerSec, 2, 1, fd);

      fseek(fd, 13, SEEK_SET);
      fread(&BPB_SecPerClus, 1, 1, fd);

      fseek(fd, 14, SEEK_SET);
      fread(&BPB_RsvdSecCnt, 2, 1, fd);

      fseek(fd, 16, SEEK_SET);
      fread(&BPB_NumFATS, 1, 1, fd);

      fseek(fd, 36, SEEK_SET);
      fread(&BPB_FATSz32, 4, 1, fd);

      fseek(fd, 40, SEEK_SET);
      fread(&BPB_ExtFlags, 2, 1, fd);

      fseek(fd, 44, SEEK_SET);
      fread(&BPB_RootClus, 4, 1, fd);

      fseek(fd, 48, SEEK_SET);
      fread(&BPB_FSInfo, 2, 1, fd);

      if (token[1] == NULL)                       //  if file does not exist
      {
        printf("%s", error_message);
      }

      else                                        //  pull the file from FAT into current directory
      {
        //  First, take user input for the file
        char file_name[MAX_FILE_NAME];
        strcpy(file_name, token[1]);

        // convert the file name input to the format it's listed in fat32
        char expanded_name[12];
        memset(expanded_name, ' ', 12);

        char *new_token = strtok(file_name, ".");
        strncpy(expanded_name, new_token, strlen(new_token));

        new_token = strtok(NULL, ".");

        if (new_token)
        {
          strncpy((char*)(expanded_name+8), new_token, strlen(new_token));
        }

        // add null terminator
        expanded_name[11] = '\0';

        // convert letters to uppercase
        for (int i= 0; i < 11; i++)
        {
          expanded_name[i] = toupper(expanded_name[i]);
        }

        //  check conversion output
        //  printf("%s", expanded_name);

        //  compare input withe directory listings
        for(int i = 0; i < 16; i++)
        {
          struct DirectoryEntry entry = dir[i];

          // only show file if their attributes are 0x01, 0x10, or 0x20
          if(entry.DIR_Attr == 0x01 || entry.DIR_Attr == 0x10 || entry.DIR_Attr == 0x20)
          {
            if (!strncmp(entry.DIR_Name, expanded_name, 11))
            {
              printf("File comparison successful.\n");

              int bytes_to_copy = entry.DIR_FileSize;

              int cluster = entry.DIR_FirstClusterLow;
              FILE *newfile = fopen(token[1], "w");
              while (bytes_to_copy > BPB_BytesPerSec)
              {
                int offset = LBAToOffset(cluster);
                fseek(fd, offset, SEEK_SET);

                char buffer[BPB_BytesPerSec];

                fread(buffer, 1, BPB_BytesPerSec, fd);
                fwrite(buffer, 1, BPB_BytesPerSec, newfile);
                cluster = NextLB(cluster);

              bytes_to_copy -= BPB_BytesPerSec;
              }

              if (bytes_to_copy > 0)
              {
                int cluster = entry.DIR_FirstClusterHigh << 16 | entry.DIR_FirstClusterLow;
                int offset = LBAToOffset(cluster);

                fseek(fd, offset, SEEK_SET);
                char buffer[BPB_BytesPerSec];

                fread(buffer, 1, bytes_to_copy, fd);
                fwrite(buffer, 1, bytes_to_copy, newfile);
                bytes_to_copy = 0;
              }

              fclose(newfile);
              printf("File retrieved %p \n", newfile);
            }
          }
        }
      }
    }

    free(head_ptr);

  }
  return 0;
}