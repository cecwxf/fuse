/*************************************************************************
	> File Name: orionfs.c
	> Author: 
	> Mail: 
	> Created Time: 2019年02月15日 星期五 16时49分11秒
 ************************************************************************/

#define FUSE_USE_VERSION 26
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<fuse.h>

#include "list.h"

#define MAX_NAMELEN 255

struct orion_entry {
    mode_t mode;
    struct list_node node;
    char name[MAX_NAMELEN +1];
};

static struct list_node entries;

static int orion_readdir(const char *path,void* buf,fuse_fill_dir_t filler,
                        off_t offset,struct fuse_file_info* fi)
{
    struct list_node* n;
    filler(buf,".",NULL,0);
    filler(buf,"..",NULL,0);

    list_for_each(n,&entries) {
        struct orion_entry* o = list_entry(n,struct orion_entry,node);
        filler(buf,o->name,NULL,0);
    }

    return 0;
}

static int orion_getattr(const char* path,struct stat* st)
{
    struct list_node* n;

    memset(st,0,sizeof(struct stat));
    if(strcmp(path,"/")==0) {
        st->st_mode = 0755|S_IFDIR;
        st->st_nlink = 2;
        st->st_size = 0;

        list_for_each(n,&entries) {
            struct orion_entry* o =list_entry(n,struct orion_entry,node);
            ++st->st_nlink;
            st->st_size +=strlen(o->name);
        }

        return 0;
    }

    list_for_each(n,&entries) {
        struct orion_entry* o=list_entry(n,struct orion_entry,node);
        if(strcmp(path+1,o->name) == 0) {
            st->st_mode = o->mode;
            st->st_nlink = 1;
            return 0;
        }
    }

    return -ENOENT;
}

static int orion_create(const char* path,mode_t mode,struct fuse_file_info* fi)
{
    struct orion_entry *o;
    struct list_node* n;
    
    if(strlen(path+1)>MAX_NAMELEN)
        return -ENAMETOOLONG;
    
    list_for_each(n,&entries) {
        o=list_entry(n,struct orion_entry,node);
        if(strcmp(path+1,o->name)==0)
            return -EEXIST;
    }

    o = malloc(sizeof(struct orion_entry));
    strcpy(o->name,path+1);
    o->mode = mode|S_IFDIR;
    list_add_prev(&o->node,&entries);

    return 0;
}

static int orion_unlink(const char* path)
{
    struct list_node *n,*p;

    list_for_each_safe(n,p,&entries){
        struct orion_entry* o =list_entry(n,struct orion_entry,node);
        if(strcmp(path+1,o->name)==0) {
            __list_del(n);
            free(o);
            return 0;
        }
    }

    return -ENOENT;
}

static struct fuse_operations orion_ops = {
    .getattr = orion_getattr,
    .readdir = orion_readdir,
    .create  = orion_create,
    .unlink  = orion_unlink,
};

int main (int args,char* argv[])
{
    list_init(&entries);
    return fuse_main(args,argv,&orion_ops,NULL);
}
