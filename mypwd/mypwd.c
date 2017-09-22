/*
    mypwd - a program demonstrating how to reconstruct the path of the
    current directory.

    Copyright (C) 2017 Hayden Walles

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    This program is part of a collection of demonstration code written
    by Hayden Walles <hayden@hexesandcursors.com> for his blog Hexes
    and Cursors <https://hexesandcursors.com>.

*/

/*

  You can use cc -o mypwd mypwd.c to compile this program.

  This is demonstration code.  It doesn't perform any error recovery.
  It shows how the system can reconstruct the name of the current
  directory and behaves much like the system command 'pwd'.

  If you want to retrieve the current directory in your own code you
  should use the 'getcwd' function or one of its friends.

  See the blog post at <https://hexesandcursors.com/misc/current-directory-demo/> for more information.

*/

/*For perror, printf, fprintf, stderr*/
#include <stdio.h>

/*For exit, free*/
#include <stdlib.h>

/*For errno*/
#include <errno.h>

/*For strdup*/
#include <string.h>

/*For fdopendir, readdir, closedir, open, fstat, openat*/
#include <sys/types.h>

/*For fstat*/
#include <sys/stat.h>

/*For fdopendir, readdir, closedir*/
#include <dirent.h>

/*For fstat*/
#include <unistd.h>

/*For open, openat*/
#include <fcntl.h>

/*
The die() function and die_if() macro are used to bail out if
something goes wrong.  This is just demonstration code, and it saves
us cluttering things up with error handling.
*/
#define s_str(x) #x
#define str(x) s_str(x)
#define die_if(x) if(x) die("Fatal error at " __FILE__ ":" str(__LINE__)" because '"  #x "'")

void die(char *msg){
  perror(msg);
  exit(EXIT_FAILURE);
}


/*
  openParent takes an open file descriptor that refers to a directory
  and opens a descriptor to its parent directory, or a negative number
  on error.
*/
int openParent(int fd){
  return openat(fd,"..",O_RDONLY|O_DIRECTORY);
}


/*
  findEntryByInode takes an open directory stream pointer and an inode
  number and returns a pointer to the directory entry that refers to
  that inode number, or NULL if there is no such entry.
*/
struct dirent *findEntryByInode(DIR *dirptr,ino_t inode){
  struct dirent *deptr;
  do {
    
    errno=0;
    deptr=readdir(dirptr);

    die_if(!deptr && errno);

    else if(deptr && deptr->d_ino==inode)
      break;
  } while(deptr);
  
  return deptr;
}

/*
  findNameByInode takes an open file descriptor referring to a
  directory and an inode number.  It returns a pointer to a
  dynamically allocated string containing the name of the entry in the
  directory referring to the given inode number, or NULL if there is
  no entry.  It closes the file descriptor.
*/
char *findNameByInode(int fd, ino_t inode){
  DIR *dir;
  char *comp;
  struct dirent *deptr;
  
  dir=fdopendir(fd);
  die_if(dir==NULL);

  
  deptr=findEntryByInode(dir,inode);
  die_if(!deptr);
  
  comp=strdup(deptr->d_name);
  die_if(!comp);
  closedir(dir);

  return comp;
}


/*
  printPathToChild is a recursive function that takes an open
  descriptor to a directory and the inode number of a directory that
  we believe is a child of that directory.  It also closes the open
  descriptor.  Its effect is to print the absolute path to the child.
*/
void printPathToChild(int parentfd, ino_t childinode){
  struct stat st;
  
  die_if(fstat(parentfd, &st)!=0);
  
  
  if(st.st_ino!=childinode){
    int gparentfd;
    char *comp;

    gparentfd=openParent(parentfd);
    die_if(gparentfd<0);

    comp=findNameByInode(parentfd,childinode);
    die_if(comp==NULL);

    printPathToChild(gparentfd,st.st_ino);

    printf("/%s",comp);
    free(comp);
  }    
  
}


/*
  printDirName takes an open descriptor to a directory and prints the
  absolute path to the directory followed by a newline.
*/
void printDirName(int fd){
  int parentfd;
  struct stat st;
  
  die_if(fstat(fd, &st)!=0);
  
  parentfd=openParent(fd);
  die_if(parentfd<0);

  printPathToChild(parentfd, st.st_ino);
  printf("\n");
}    
      
 
int main(int argc, char **argv){
  int fd;

  die_if(argc!=1);
    
  fd=open(".",O_RDONLY|O_DIRECTORY);
  die_if(fd<0);

  printDirName(fd);
  close(fd);
  
  return EXIT_SUCCESS;
}
