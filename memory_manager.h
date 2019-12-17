

typedef struct FIFOframNode
{
    int framNum;    //physical fram
    int virtNum;    //virtual fram number
    struct FIFOframNode* next;
} FNode;


typedef struct ESCAframNode
{
    int framNum;
    int virtNum;
    unsigned char ref;
    unsigned char dir;
    struct ESCAframNode* last;
    struct ESCAframNode* next;
} ENode;


typedef struct SLRUframNode
{
    int framNum;
    int virtNum;
    unsigned char ref;
    unsigned char act;
    struct SLRUframNode* last;
    struct SLRUframNode* next;
} SNode;

typedef struct ANSNode
{
    int evVir;
    int evPag;
    int idVir;
    int idPag;
    int idSou;
    int framLen;
    int frameActLen;
    unsigned char HitMiss;
} ANS;