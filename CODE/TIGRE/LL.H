/*
Copyright (C) Magna Carta Software, Inc. 1992-1993. All Rights Reserved.
C COMMUNICATIONS TOOLKIT
LL.H -- Routines for handling linked lists of the form:
struct mclink {
    struct mclink   *next;
    struct mclink   *prev;
    ...
};
Note the ordering of 'next' and 'prev'. The 'mc' refers to 'Magna Carta'
and exists solely to prevent name conflicts with your other code.
*/

#if !defined(LL_H_INCLUDED)
    #define LL_H_INCLUDED
    short           FCALL_ ll_append_link(void *base, void  *node);
    unsigned long   FCALL_ ll_count_links(void  *base);
    short           FCALL_ ll_delete_link(void  *node);
    short           FCALL_ ll_delete_list(void  *node);
    void  * FCALL_ ll_find_link(void  *base, void  *node);
    void  * FCALL_ ll_find_linknum(void  *base, unsigned short pos);
    short           FCALL_ ll_insert_link(void  *base, void  *node);
#endif
