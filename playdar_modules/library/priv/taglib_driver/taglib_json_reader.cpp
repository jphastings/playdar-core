/*
    An erlang driver for taglib that outputs JSON.

    This process is passed filenames on stdin (from erlang)
    It uses taglib on the file, and outputs JSON containing the
    metadata, according to taglib. One line of JSON output per file.

*/
#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <cstdio>
#include <sstream>
#include <algorithm>
#include <string>

#ifdef WIN32
#pragma comment(lib, "ws2_32.lib")
#include <Winsock2.h>
#else
#include <netinet/in.h> // for htonl etc
#endif

using namespace std;

#ifdef WIN32
wstring fromUtf8(const string& i);
string toUtf8(const wstring& i);
#else
#define fromUtf8(s) (s)
#define toUtf8(s) (s)
#endif

void trim(string& str)
{
    size_t startpos = str.find_first_not_of(" \t");
    size_t endpos = str.find_last_not_of(" \t");
    if(( string::npos == startpos ) || ( string::npos == endpos))
        str = "";
    else
        str = str.substr( startpos, endpos-startpos+1 );
}

string urlify(const string& p)
{
    // turn it into a url by prepending file://
    // because we pass all urls to curl:
    string urlpath("file://");
	if (p.at(0)=='/' || p.at(1)==':') // posix path starting with / or windows style filepath
    {
        urlpath += p;
    }
    else urlpath = p;

    return urlpath;
}

string ext2mime(const string& ext)
{
    if(ext==".mp3") return "audio/mpeg";
    if(ext==".aac") return "audio/mp4";
    if(ext==".mp4") return "audio/mp4";
    if(ext==".m4a") return "audio/mp4";
    if(ext==".ogg") return "application/ogg";
    cerr << "Warning, unhandled file extension. Don't know mimetype for " << ext << endl;
     //generic:
    return "application/octet-stream";
}

// replace whitespace and other control codes with ' ' and replace multiple whitespace with single
string tidy(const string& s)
{
    string r;
    bool prevWasSpace = false;
    r.reserve(s.length());
    for (string::const_iterator i = s.begin(); i != s.end(); i++) {
        if (*i > 0 && *i <= ' ') {
            if (!prevWasSpace) {
                r += ' ';
                prevWasSpace = true;
            }
        } else if (*i == '"') {
            r += '\\';
            r += '"';
            prevWasSpace = false;
        } else {
            r += *i;
            prevWasSpace = false;
        }
    }
    return r;
}

string esc(const string& s)
{
    string r;
    bool prevWasSpace = false;
    r.reserve(s.length()+4);
    for (string::const_iterator i = s.begin(); i != s.end(); i++) {
        if (*i == '"') {
            r += '\\';
            r += '"';
            prevWasSpace = false;
        } else {
            r += *i;
            prevWasSpace = false;
        }
    }
    return r;
}

string scan_file(const char* path)
{
    TagLib::FileRef f(path);
    if (!f.isNull() && f.tag()) {
        TagLib::Tag *tag = f.tag();
        int bitrate = 0;
        int duration = 0;
        if (f.audioProperties()) {
            TagLib::AudioProperties *properties = f.audioProperties();
            duration = properties->length();
            bitrate = properties->bitrate();
        }
        string artist = tag->artist().toCString(true);
        string album  = tag->album().toCString(true);
        string track  = tag->title().toCString(true);
        trim(artist);
        trim(album);
        trim(track);
        if (artist.length()==0 || track.length()==0) {
            return "{\"error\" : \"no tags\"}";
        }
        string pathstr(path);
        string ext = pathstr.substr(pathstr.length()-4);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        string mimetype = ext2mime(ext);
        // turn it into a url by prepending file://
        // because we pass all urls to curl:
        string urlpath = urlify( path );

        ostringstream os;
        os      <<  "{  \"url\" : \"" << esc(urlpath) << "\","
                    "   \"mimetype\" : \"" << mimetype << "\","
                    "   \"artist\" : \"" << tidy(artist) << "\","
                    "   \"album\" : \"" << tidy(album) << "\","
                    "   \"track\" : \"" << tidy(track) << "\","
                    "   \"duration\" : " << duration << ","
                    "   \"bitrate\" : " << bitrate << ","
                    "   \"trackno\" : " << tag->track()
                <<  "}";
        return os.str();
    }
    return "{\"error\" : \"no tags\"}";
}

#ifdef WIN32
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    //scan_file( argv[1] );
    char buffer[256];
    unsigned int len0,len;

    while(1)
    {
        if(fread(&len0, 4, 1, stdin)!=1) break;
        len = ntohl(len0);

        fread(&buffer, 1, len, stdin);
        buffer[len]='\0';

        string j = scan_file((const char*)&buffer);
        unsigned int l = htonl(j.length());

        fwrite(&l,4,1,stdout);
        printf("%s", j.c_str());
        fflush(stdout);
    }
    return 0;
}
