#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory_manager.h"

ANS FIFO_manager(int* vfram, int* vDisk, int* diskR, int framLen, int framNum, int dSize, int ID, ANS ans);
ANS ESCA_manager(int* vfram, int* vDisk, int* diskR, int framLen, int framNum, int dSize, int ID, unsigned char RW, ANS ans);
ANS SLRU_manager(int* vfram, int* vDisk, int* diskR, int framLen, int framNum, int dSize, int ID, int frameActLen, int frameActNum, ANS ans);
ANS getAns(ANS ans, int evVir, int evPag, int idVir, int idPag, int idSou, int framLen, int frameActLen, unsigned char HitMiss);

int dealStr(char* str, int k);



FNode* fhead = NULL;
FNode* ftail = NULL;
ENode* ehead = NULL;
ENode* etail = NULL;
SNode* shead = NULL;
SNode* stail = NULL;
SNode* sheadAct = NULL;
SNode* stailAct = NULL;



int main()
{

    FILE* fr;
    FILE* fw;
    int framLen = 0;     //the amount of now occupied fram
    int frameActLen = 0;  //the amount of now occupied active fram
    int framNum = -1;    //total fram number
    int frameActNum = 0; //total active fram number
    int virtual = -1;    //total virtual fram number
    int policyN = -1;    //0: FIFO, 1:enhanced, 2:SLRU

    if((fr = fopen("input.txt","r") ) == NULL )
    {
        perror("open input file failed: ");
        return -1;
    }
    if((fw = fopen("output.txt","w") ) == NULL )
    {
        perror("open output file failed: ");
        return -1;
    }

    int i = 0;
    char str[60];
    char *substr;
    char myAnsStr[100];
    const char *del = ":";
    const char *del2 = " ";

    //for calculate page fault rate
    float all_instr = 0;
    float miss = 0;

    for(i = 0; i < 3; i++)
    {
        if(fgets(str, 60,fr) != NULL)
        {
            //get policyN, framNum, virtual
            substr = strtok(str, del);
            substr = strtok(NULL, del);
            if(i == 0)
                policyN = dealStr(substr,i);
            else if(i == 1)
                virtual = dealStr(substr,i);
            else
                framNum = dealStr(substr,i);
            memset(str,'\0', 60);
        }
        memset(str,'\0', 60);
    }
    int dSize = virtual-framNum+5;
    int vfram[virtual];
    int vDisk[virtual];
    int diskR[dSize];
    for(i = 0; i < virtual; i++)
    {
        vfram[i] = -1;
        vDisk[i] = -1;
    }
    for(i = 0; i < dSize; i++)
    {
        diskR[i] = -1;
    }
    if(fgets(str, 60,fr) == NULL)
        return -1;

    // get instr from input.txt
    while(fgets(str, 60,fr) != NULL)
    {
        ANS ans;
        int ID = -1;
        //strtok get Instr, ID
        substr = strtok(str, del2);
        substr = strtok(NULL, del2);
        ID = dealStr(substr,3);
        if(ID > virtual)
        {
            printf("out of virtual fram size\n");
            return -1;
        }
        if(policyN == 0 || framNum == 1)
        {
            ans = FIFO_manager(vfram, vDisk, diskR, framLen, framNum, dSize, ID, ans);
            framLen = ans.framLen;
        }
        else if(policyN == 1)
        {
            unsigned char RW = '0';
            if(strcmp(str,"Write") == 0)
                RW = '1';
            ans = ESCA_manager(vfram, vDisk, diskR, framLen, framNum, dSize, ID, RW, ans);
            framLen = ans.framLen;
        }
        else if(policyN == 2)
        {

            //frameLen = inActframe

            frameActNum = framNum/2;
            ans = SLRU_manager(vfram, vDisk, diskR, framLen, framNum, dSize, ID, frameActLen, frameActNum, ans);
            framLen = ans.framLen;
            frameActLen = ans.frameActLen;
        }
        if(ans.HitMiss == '1')
        {
            //Hit
            //printf("Hit, %d=>%d\n", ans.idVir, ans.idPag);
            sprintf(myAnsStr,"Hit, %d=>%d\n",ans.idVir, ans.idPag);
        }
        else
        {
            //Miss
            miss++;
            //printf("Miss, %d, %d>>%d, %d<<%d\n",ans.idPag, ans.evVir, ans.evPag,ans.idVir, ans.idSou);
            sprintf(myAnsStr, "Miss, %d, %d>>%d, %d<<%d\n",ans.idPag, ans.evVir, ans.evPag,ans.idVir, ans.idSou);
        }

        fputs(myAnsStr, fw);
        memset(str,'\0', 60);
        memset(myAnsStr,'\0', 100);

        //made page table
        // PFI/DBI : vfram/vDisk
        // in Use  : 0 :if(vfram == -1 && vDisk == -1) / 1 :else
        // present : 0 :if(vfram == -1) / 1 :else
        //count page fault
        all_instr ++;

        //write to fw

    }
    sprintf(myAnsStr,"Page Fault Rate: %.3f\n",miss/all_instr);

    //write page fault rate to fw
    fputs(myAnsStr, fw);

    memset(myAnsStr,'\0', 100);

    //close file
    fclose(fr);
    fclose(fw);



    return -1;

}



ANS FIFO_manager(int* vfram, int* vDisk, int* diskR, int framLen, int framNum, int dSize, int ID, ANS ans)
{
    if(vfram[ID] != -1)
    {
        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, -1, -1, ID, vfram[ID], vDisk[ID], framLen, -1, '1');
        return ans;
    }
    if(framLen < framNum)
    {
        //still have room in frame
        if(framLen == 0)
        {
            fhead = malloc(sizeof(FNode));
            ftail = fhead;
        }
        else
        {
            ftail->next = malloc(sizeof(FNode));
            ftail = ftail->next;
        }

        ftail->framNum = framLen;
        ftail->virtNum = ID;
        ftail->next = NULL;
        vfram[ID] = ftail->framNum;
        framLen++;

        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, -1, -1, ID, ftail->framNum, vDisk[ID], framLen, -1, '0');
    }
    else
    {
        //page replacement
        ftail->next = malloc(sizeof(FNode));
        ftail = ftail->next;

        ftail->framNum = fhead->framNum;
        ftail->virtNum = ID;
        vfram[ID] = ftail->framNum;
        vfram[fhead->virtNum] = -1;

        int i = 0;

        //find free disk to store the replaced one
        for(i = 0; i < dSize; i++)
        {
            if(diskR[i] == -1)
            {
                diskR[i] = fhead->virtNum;
                vDisk[fhead->virtNum] = i;
                break;
            }
        }
        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, fhead->virtNum, vDisk[fhead->virtNum], ID, vfram[ID], vDisk[ID], framLen, -1, '0');

        //release occupied(by ID) disk
        if(vDisk[ID] != -1)
        {
            diskR[vDisk[ID]] = -1;
            vDisk[ID] = -1;
        }
        FNode* fptr = fhead;
        fhead = fhead->next;
        free(fptr);
    }

    return ans;

}



ANS ESCA_manager(int* vfram, int* vDisk, int* diskR, int framLen, int framNum, int dSize, int ID, unsigned char RW, ANS ans)
{

    if(vfram[ID] != -1)
    {
        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, -1, -1, ID, vfram[ID], vDisk[ID], framLen, -1, '1');

        int i = 0;
        ENode* eptr = ehead;
        for(i = 0; i < framLen; i++)
        {
            if(eptr->virtNum == ID)
            {
                break;
            }
            eptr = eptr->next;
        }
        if(RW == '1')
            eptr->dir = '1';
        eptr->ref = '1';

        return ans;
    }
    if(framLen < framNum)
    {
        /* physical memory is not full */
        if(framLen == 0)
        {
            ehead = malloc(sizeof(ENode));
            etail = ehead;
        }
        else
        {
            etail->next = malloc(sizeof(ENode));
            etail->next->last = etail;
            etail = etail->next;
        }

        etail->ref = '1';
        etail->dir = RW;
        etail->framNum = framLen;
        etail->virtNum = ID;
        etail->next = NULL;

        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, -1, -1, ID, framLen, vDisk[ID], framLen+1, -1, '0');
        vfram[ID] = framLen;
        framLen++;
    }
    else
    {

        /* physical memory is full */
        etail->next = malloc(sizeof(ENode));
        etail->next->last = etail;
        etail = etail->next;
        ENode* eptr = ehead;
        ENode* eptrr = NULL;
        ENode* eptrd = NULL;

        etail->virtNum = ID;




        /* choose evicted */
        int i = 0;
        for(i = 0; i < 2; i++)
        {
            int j = 0;
            eptr = ehead;
            for(j= 0; j< framLen; j++)
            {
                if(eptr->ref == '0')
                {
                    if(eptr->dir == '0')
                    {
                        eptrr = eptr;
                        break;
                    }
                    else if(eptrd == NULL)
                    {
                        eptrd = eptr;
                    }
                }
                else if(eptr->virtNum != ID)
                {
                    eptr->ref = '0';
                }
                eptr = eptr->next;
            }

        }
        if(eptrd != NULL)
        {
            eptr = eptrd;
        }
        if(eptrr != NULL)
        {
            eptr = eptrr;
        }

        /* switch node */
        etail->ref = '1';
        etail->dir = RW;
        etail->framNum = eptr->framNum;
        etail->virtNum = ID;
        etail->next = NULL;

        vfram[ID] = etail->framNum;
        vfram[eptr->virtNum] = -1;

        for(i = 0; i < dSize; i++)
        {
            if(diskR[i] == -1)
            {
                diskR[i] = eptr->virtNum;
                vDisk[eptr->virtNum] = i;
                break;
            }
        }

        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, eptr->virtNum, vDisk[eptr->virtNum], ID, vfram[ID], vDisk[ID], framLen, -1, '0');

        if(vDisk[ID] != -1)
        {
            diskR[vDisk[ID]] = -1;
            vDisk[ID] = -1;
        }

        if(eptr->last == NULL)
        {
            ehead = ehead->next;
        }
        else
        {
            eptr->last->next = eptr->next;
            eptr->next->last = eptr->last;
        }
        free(eptr);

        eptrr = NULL;
        eptrd = NULL;



    }






    return ans;
}



ANS SLRU_manager(int* vfram, int* vDisk, int* diskR, int framLen, int framNum, int dSize, int ID, int frameActLen, int frameActNum, ANS ans)
{
    if(vfram[ID] != -1)
    {
        stail->next = NULL;
        SNode* sptr = shead;
        int i = 0;
        for(i = 0; i < framLen; i++)
        {
            if(sptr->virtNum == ID)
            {
                break;
            }
            sptr = sptr->next;

        }
        if(sptr == NULL)
        {
            sptr = sheadAct;
            for(i = 0; i < frameActLen; i++)
            {
                if(sptr->virtNum == ID)
                {
                    break;
                }
            }
        }
        if(sptr->ref == '0')
        {
            sptr->ref = '1';
            if(sptr->act == '0')
            {
                //[ref, act] = [0,0]
                if(sptr != shead)
                {
                    //if sptr == shead, no need to change
                    //if sptr != shead, sptr need to move to head
                    sptr->last->next = sptr->next;
                    sptr->next->last = sptr->last;
                    shead->last = sptr;
                    shead = sptr;
                }
            }
            else
            {
                //[ref, act] = [0,1]
                if(sptr != sheadAct)
                {
                    //if sptr == sheadAct, no need to change
                    //if sptr != sheadAct, sptr need to move to head
                    sptr->last->next = sptr->next;
                    sptr->next->last = sptr->last;
                    sheadAct->last = sptr;
                    sheadAct = sptr;
                }
            }
        }
        else
        {
            //ref == 1
            if(sptr->act == '0')
            {

                //[ref, act] = [1,0]
                // move to active list head(frameNum also need to change)
                // if act list is full, move one to inactive
                // if inact list is full, release one

                //move sptr to active list from inactive one
                if(sheadAct != NULL)
                {
                    sheadAct->last = sptr;
                    sheadAct = sptr;
                }
                else
                {
                    //no node in active list
                    sheadAct = sptr;
                    stailAct = sptr;
                }

                if(sptr != shead)
                {
                    sptr->last->next = sptr->next;
                    sptr->next->last = sptr->last;
                }
                else
                {
                    shead = shead->next;
                }

                // reset framelen
                frameActLen++;
                framLen--;


                // if active list is out of size, resize
                if(frameActLen > frameActNum)
                {
                    //find node in active list which ref = 0
                    SNode* sreptr = stailAct;
                    while(1)
                    {
                        if(sreptr->ref == '1')
                        {
                            // if stail->ref == '1', change to '0' and move to head
                            sreptr->ref = '0';
                            if(frameActLen == 1)
                            {
                                break;
                            }
                            else
                            {
                                stailAct = stailAct->last;
                                stailAct->next = NULL;
                                sreptr->next = sheadAct;
                                sheadAct->last = sreptr;
                                sheadAct = sheadAct->last;
                                sreptr = stailAct;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }

                    //vfram[sptr->virtNum] = sptr->framNum;
                    sptr->act = '1';

                    // active list is out of size, move out stailAct
                    sreptr = stailAct;
                    stailAct = stailAct->last;
                    stailAct->next = NULL;


                    if(shead == NULL)
                    {
                        shead = sreptr;
                        stail = sreptr;
                    }
                    else
                    {
                        sreptr->next = shead;
                        shead->last = sreptr;
                        shead = shead->last;
                    }


                    //reset framename, act to inactive list
                    sreptr->ref = '0';
                    sreptr->act = '0';


                    // reset framelen
                    frameActLen--;
                    framLen++;

                }
                //change frame to active frame
                //reset frame and act
                sptr->ref = '0';
                sptr->act = '1';

            }
            else
            {
                //[ref, act] = [1,1]
                if(sptr != sheadAct)
                {
                    //if sptr == sheadAct, no need to change
                    //if sptr != sheadAct, sptr need to move to head
                    sptr->last->next = sptr->next;
                    sptr->next->last = sptr->last;
                    sheadAct->last = sptr;
                    sheadAct = sptr;
                }
            }

        }
        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, -1, -1, ID, vfram[ID], vDisk[ID], framLen, frameActLen, '1');
        return ans;
    }


    if(framLen < (framNum - frameActNum))
    {
        //still have room & ID never record
        if(framLen == 0)
        {
            shead = malloc(sizeof(SNode));
            stail = shead;
        }
        else
        {
            shead->last = malloc(sizeof(SNode));
            shead->last->next = shead;
            shead = shead->last;
        }

        shead->framNum = framLen+frameActLen;
        shead->virtNum = ID;
        shead->ref = '1';
        shead->act = '0';
        shead->last = NULL;

        vfram[ID] = shead->framNum;
        framLen++;



        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, -1, -1, ID, shead->framNum, vDisk[ID], framLen, frameActLen, '0');
    }
    else
    {
        //page replacement

        // find a snode which ref = 0
        SNode* sptr = stail;
        while(1)
        {
            if(sptr->ref == '1')
            {
                // if stail->ref == '1', change to '0' and move to head
                sptr->ref = '0';
                if(framLen == 1)
                {
                    break;
                }
                else
                {
                    stail = stail->last;
                    stail->next = NULL;
                    sptr->next = shead;
                    sptr->last = NULL;
                    shead->last = sptr;
                    shead = shead->last;
                    sptr = stail;
                }
            }
            else
            {
                break;
            }
        }
        //create new node for ID(at shead)
        shead->last = malloc(sizeof(SNode));
        shead->last->next = shead;
        shead = shead->last;




        //ID get free frame from sptr
        shead->framNum = sptr->framNum;
        shead->virtNum = ID;
        shead->ref = '1';
        shead->act = '0';


        //record frame in vPage
        vfram[ID] = shead->framNum;

        //remove stail frame record from vPage
        vfram[sptr->virtNum] = -1;

        int i = 0;

        // put stail in the first free frame of disk
        // & record in vDisk
        for(i = 0; i < dSize; i++)
        {
            if(diskR[i] == -1)
            {
                diskR[i] = sptr->virtNum;
                vDisk[sptr->virtNum] = i;
                break;
            }
        }


        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, sptr->virtNum, vDisk[sptr->virtNum], ID, vfram[ID], vDisk[ID], framLen, frameActLen, '0');
        // change ID source
        if(vDisk[ID] != -1)
        {
            diskR[vDisk[ID]] = -1;
            vDisk[ID] = -1;
        }



        // delete the tail of inactive list
        sptr = stail;
        stail = stail->last;
        free(sptr);


    }

    return ans;
}




ANS getAns(ANS ans, int evVir, int evPag, int idVir, int idPag, int idSou, int framLen, int frameActLen, unsigned char HitMiss)
{
    ans.HitMiss = HitMiss;
    ans.framLen = framLen;
    ans.evVir = evVir;
    ans.evPag = evPag;
    ans.idVir = idVir;
    ans.idPag = idPag;
    ans.frameActLen = frameActLen;
    ans.idSou = idSou;
    return ans;
}




int dealStr(char* str, int k)
{
    if(str == NULL)
        return -1;
    char res[strlen(str)];
    strcpy(res,str);
    memset(str,'\0', strlen(str));
    int i = 0,j = 0;;
    for(i = 0; i < strlen(res); i++)
    {
        if(res[i] != ' ' && res[i] != '\n')
        {
            str[j] = res[i];
            j++;
        }
        else if(res[i] == '\n')
            str[j] = '\0';
    }
    if(k == 0)
    {
        if(strcmp("FIFO",str) == 0)
        {
            return 0;
        }
        else if(strcmp("ESCA",str) == 0)
        {
            return 1;
        }
        else if(strcmp("SLRU",str) == 0)
        {
            return 2;
        }
    }
    else if(k == 1 || k == 2 || k == 3)
    {
        return atoi(str);
    }
    return -1;
}
