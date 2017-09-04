#ifndef UOANIMMUL_H
#define UOANIMMUL_H

#include <string>
#include <unordered_map>


class QImage;

class UOAnimMul
{
private:
    std::string m_clientPath;

    class BodyDefEntry
    {
    public:
        int newID = 0;
        int newHue = 0;
        BodyDefEntry(int NewID, int NewHue)
        {
            newID = NewID;
            newHue = NewHue;
        }
    };
    class BodyConvDefEntry
    {
    public:
        int newID = 0;
        int newFileNum = 0;
        BodyConvDefEntry(int NewID, int NewFileNum)
        {
            newID = NewID;
            newFileNum = NewFileNum;
        }
    };

    std::unordered_map<int,BodyDefEntry>       m_bodyDef;       // lookup key (int): oldID
    std::unordered_map<int,BodyConvDefEntry>   m_bodyConvDef;   // lookup key (int): oldID

    bool loadBodyDef();
    bool loadBodyConvDef();

    unsigned int getBodyLookupIndex(int body, int action, int direction, int animFileNumber);   // return the in-file index of the anim we want to show

public:
    UOAnimMul(std::string clientPath);
    QImage* drawAnimFrame(int bodyID, int action, int direction, int frame, int hueIndex);
};

#endif // UOANIMMUL_H