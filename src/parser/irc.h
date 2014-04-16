/* This file was automatically generated.  Do not edit! */
#define irc_ParseTOKENTYPE void*
#define irc_ParseARG_PDECL
void irc_Parse(void *yyp,int yymajor,irc_ParseTOKENTYPE yyminor irc_ParseARG_PDECL);
#if defined(YYTRACKMAXSTACKDEPTH)
int irc_ParseStackPeak(void *p);
#endif
void irc_ParseFree(void *p,void(*freeProc)(void *));
void *irc_ParseAlloc(void *(*mallocProc)(size_t));
#if !defined(NDEBUG)
void irc_ParseTrace(FILE *TraceFILE,char *zTracePrompt);
#endif
#define irc_ParseARG_STORE
#define irc_ParseARG_FETCH
#define irc_ParseARG_SDECL
#define IRC_TRAILING                       15
#define IRC_PARAM                          14
#define IRC_CMD_PRIVMSG                    13
#define IRC_CMD_QUIT                       12
#define IRC_CMD_PONG                       11
#define IRC_CMD_PING                       10
#define IRC_HOST                            9
#define IRC_AT                              8
#define IRC_USER                            7
#define IRC_EXCLAMATION                     6
#define IRC_SERVERNAME                      5
#define IRC_SPACE                           4
#define IRC_NICKNAME                        3
#define IRC_COLON                           2
#define IRC_CRLF                            1
#define INTERFACE 0
