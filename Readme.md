This is a simple implementation of Git 

Future Scope :

1. Adding makefile 
2. Rewrite of checkout 

To run please compile to object file :

`gcc -o git.o vcs.c`

It will generate a git.o object file 
create a alias so you dont need to run everytime 

Now to use it
1. Initilize a repository using 
    `git init`
2. add any files to staging area using 
    `git add <file1name> <file2name> ...`
3. To check what are being tracked 
    `git status` 
3. commit any changes using 
    `git commit -m "commit message"`
4. To see all commits 
    `git log` 
5. To revert back changes (needs improvement)
    currently it is only reverting changes only to that commit 
    `git checkout commithash`






