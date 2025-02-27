//based on core_api from tempura
class WReader 
{
  protected:

    /* WReader
    ** WReader constructor
    */
    WReader() { }
  
  public:

    /* m_player
    ** Filled by Winamp. Pointer to Winamp 3 core interface
    */
    //WPlayer_callback *m_player;

    /* GetDescription
    ** Retrieves your plug-in's text description
    */
    virtual char *GetDescription() { return (char*)"Unknown"; };

    /* Open
    ** Used to open a file, return 0 on success
    */
    virtual int Open(char *url, bool *killswitch)=0; 

    /* Read
    ** Returns number of BYTES read (if < length then eof or killswitch)
    */
    virtual int Read(char *buffer, int length, bool *killswitch)=0; 
                                                                
    /* GetLength
    ** Returns length of the entire file in BYTES, return -1 on unknown/infinite (as for a stream)
    */
    virtual int GetLength(void)=0; 
    
    /* CanSeek
    ** Returns 1 if you can skip ahead in the file, 0 if not
    */
    virtual int CanSeek(void)=0;
    
    /* Seek
    ** Jump to a certain absolute position
    */
    virtual int Seek(int position, bool *killswitch)=0;
    
    /* GetHeader
    ** Retrieve header. Used in read_http to retrieve the HTTP header
    */
    virtual char *GetHeader(char *name) { return 0; }

    /* ~WReader
    ** WReader virtual destructor
    */
    virtual ~WReader() { }
};




#define READ_VER    0x100


typedef struct 
{
    /* version
    ** Version revision number
    */
    int version;
    
    /* description
    ** Text description of the reader plug-in
    */
    char *description;
    
    /* create
    ** Function pointer to create a reader module
    */
    WReader *(*create)();
    
    /* ismine
    ** Determines whether or not a file should be read by this plug-in
    */
    int (*ismine)(char *url);
    
} reader_source;

/*
exported symbols:
int _cdecl readerSource(HINSTANCE hIns,reader_source** s);
(retrieves reader_source struct)

int _stdcall gzip_writefile(char* path,void* buf,DWORD size);
(writes gzip file, very handy in infobox)
*/
