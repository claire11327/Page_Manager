#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory_manager.h"

ANS FIFO_manager(int* vfram, int* vDisk, int* diskR, int framLen, int framNum, int ID, ANS ans);
ANS ESCA_manager(int* vfram, int* vDisk, int* diskR, int framLen, int framNum, int ID, unsigned char RW, ANS ans);
ANS SLRU_manager(int* vfram, int* vDisk, int* diskR, int framLen, int framNum, int ID, int frameActLen, int frameActNum, ANS ans);
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
    int vfram[virtual];
    int vDisk[virtual];
    int diskR[virtual-framNum];
    for(i = 0; i < virtual; i++)
    {
        vfram[i] = -1;
        vDisk[i] = -1;
    }
    for(i = 0; i < (virtual - framNum); i++)
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
        if(policyN == 0)
        {
            ans = FIFO_manager(vfram, vDisk, diskR, framLen, framNum, ID, ans);
            framLen = ans.framLen;
        }
        else if(policyN == 1)
        {
            unsigned char RW = '0';
            if(strcmp(str,"Write") == 0)
                RW = '1';
            ans = ESCA_manager(vfram, vDisk, diskR, framLen, framNum, ID, RW, ans);
            framLen = ans.framLen;
        }
        else if(policyN == 2)
        {

            //frameLen = inActframe
            frameActNum = framNum - framNum/2;
            ans = SLRU_manager(vfram, vDisk, diskR, framLen, framNum, ID, frameActLen, frameActNum, ans);
            framLen = ans.framLen;
            frameActLen = ans.frameActLen;
        }
        if(ans.HitMiss == '1')
        {
            //Hit
            printf("Hit, %d=>%d\n", ans.idVir, ans.idPag);
            sprintf(myAnsStr,"Hit, %d=>%d\n",ans.idVir, ans.idPag);
        }
        else
        {
            //Miss
            miss++;
            printf("Miss, %d, %d>>%d, %d<<%d\n",ans.idPag, ans.evVir, ans.evPag,ans.idVir, ans.idSou);
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



ANS FIFO_manager(int* vfram, int* vDisk, int* diskR, int framLen, int framNum, int ID, ANS ans)
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

        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, -1, -1, ID, framLen, vDisk[ID], framLen+1, -1, '0');
        vfram[ID] = framLen;
        framLen++;
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
        int dSize = sizeof(diskR)/sizeof(int);

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
        ans = getAns(ans, fhead->virtNum, fhead->framNum, ID, vfram[ID], vDisk[ID], framLen, -1, '0');

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



ANS ESCA_manager(int* vfram, int* vDisk, int* diskR, int framLen, int framNum, int ID, unsigned char RW, ANS ans)
{
    if(vfram[ID] != -1)
    {
        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, -1, -1, ID, vfram[ID], vDisk[ID], framLen, -1, '1');
        if(RW == '1'){
            int i = 0;
            ENode* eptr = ehead;
            for(i = 0; i < framLen; i++){
                if(eptr->virtNum == ID){
                    break;
                }
            }
            eptr->dir = '1';
        }
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
        etail->last = NULL;

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


        /* choose evicted */
        int i = 0;
        for(i = 0; i < 2; i++)
        {
            eptr = ehead;
            while(eptr != etail)
            {

                if(eptr->ref == '0')
                {
                    if(eptr->dir == '0')
                    {
                        eptrr = eptr;
                        break;
                    }
                    else if(eptr->dir == '1' && eptrd == NULL)
                    {
                        eptrd = eptr;

                    }
                }
                else
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

        int dSize = sizeof(diskR)/sizeof(int);

        for(i = 0; i < dSize; i++)
        {
            if(diskR[i] == -1)
            {
                diskR[i] = ehead->virtNum;
                vDisk[ehead->virtNum] = i;
                break;
            }
        }

        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, eptr->virtNum, eptr->framNum, ID, vfram[ID], vDisk[ID], framLen, -1, '0');

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
        }
        free(eptr);




    }
    return ans;
}



ANS SLRU_manager(int* vfram, int* vDisk, int* diskR, int framLen, int framNum, int ID, int frameActLen, int frameActNum, ANS ans)
{
    if(vfram[ID] != -1)
    {

        int len = -1;
        SNode* sptr = NULL;
        if(vfram[ID] < (framNum/2))
        {
            //virtual page is in inactive list
            sptr = shead;
            len = framLen;
        }
        else
        {
            //virtual page is in active list
            sptr = sheadAct;
            len = frameActLen;
        }

        int i = 0;
        for(i = 0; i < len; i++)
        {
            if(sptr->virtNum == ID)
            {
                break;
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
                    sheadAct->last = sptr;
                    sheadAct = sptr;
                }
            }
        }
        else
        {
            if(sptr->act == '0')
            {
                //[ref, act] = [1,0]
                // move to active list head(frameNum also need to change)
                // if act list is full, move one to inactive
                // if inact list is full, release one
                if(sheadAct != NULL)
                {
                    sheadAct->last = sptr;
                    sheadAct = sptr;
                }
                else
                {
                    sheadAct = sptr;
                    stailAct = sptr;
                }

                if(sptr != shead)
                {
                    sptr->last->next = sptr->next;
                }
                else
                {
                    shead = shead->next;
                }

                //change frame to active frame
                //reset frame and act
                if((framNum/2)+frameActLen < framNum)
                {
                    sptr->framNum = (framNum/2)+frameActLen;
                }
                else
                {
                    sptr->framNum = framNum-1;
                }
                sptr->ref = '0';
                sptr->act = '1';
                vfram[ID] = sptr->framNum;

                frameActLen++;
                framLen--;
                if(frameActLen > frameActNum)
                {
                    // active list is out of size, move out stailAct
                    sptr = stailAct;
                    stailAct = stailAct->last;
                    stailAct->next = NULL;

                    if(shead == NULL)
                    {
                        shead = sptr;
                        stail = sptr;
                    }
                    else
                    {
                        sptr->next = shead;
                        shead->last = sptr;
                        shead = shead->last;
                    }

                    //reset framename, act to inactive list
                    sptr->framNum = framLen;
                    vfram[sptr->virtNum] = sptr->framNum;
                    sptr->act = '0';

                }
            }
            else
            {
                //[ref, act] = [1,1]
                if(sptr != sheadAct)
                {
                    //if sptr == sheadAct, no need to change
                    //if sptr != sheadAct, sptr need to move to head
                    sptr->last->next = sptr->next;
                    sheadAct->last = sptr;
                    sheadAct = sptr;
                }
            }
        }


        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, -1, -1, ID, vfram[ID], vDisk[ID], framLen, frameActLen, '1');
        return ans;
    }


    if(framLen < (framNum/2))
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
            shead = shead->last;
        }

        shead->framNum = framLen;
        shead->virtNum = ID;
        shead->ref = '0';
        shead->act = '0';
        shead->last = NULL;


        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, -1, -1, ID, framLen, vDisk[ID], framLen+1, frameActLen, '0');
        vfram[ID] = framLen;
        framLen++;
    }
    else
    {
        //page replacement

        //create new node for ID(at shead)
        shead->last = malloc(sizeof(SNode));
        shead = shead->last;

        //ID get free frame from stail
        shead->framNum = stail->framNum;
        shead->virtNum = ID;
        shead->ref = '0';
        shead->act = '0';

        //record frame in vPage
        vfram[ID] = shead->framNum;

        //remove stail frame record from vPage
        vfram[stail->virtNum] = -1;

        int i = 0;
        int dSize = sizeof(diskR)/sizeof(int);

        // put stail in the first free frame of disk
        // & record in vDisk
        for(i = 0; i < dSize; i++)
        {
            if(diskR[i] == -1)
            {
                diskR[i] = stail->virtNum;
                vDisk[stail->virtNum] = i;
                break;
            }
        }
        //(ANS ans, evVir, evPag, idVir, idPag, idSou, framLen, frameActLen, HitMiss);
        ans = getAns(ans, stail->virtNum, stail->framNum, ID, vfram[ID], vDisk[ID], framLen, frameActLen, '0');

        // change ID source
        if(vDisk[ID] != -1)
        {
            diskR[vDisk[ID]] = -1;
            vDisk[ID] = -1;
        }

        // delete the tail of inactive list
        SNode* sptr = stail;
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
