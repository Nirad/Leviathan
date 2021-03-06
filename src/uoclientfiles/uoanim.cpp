#include "uoanim.h"

namespace uocf
{

UOAnim::UOAnim(const std::string &clientPath, std::function<void(int)> reportProgress) :
    m_UOAnimMUL(clientPath), m_UOAnimUOP(clientPath, reportProgress)
{
    // Do not pass as a const reference reportProgress to the constructor: be sure to have at least one copy of it, in case
    //  the referred object isn't valid after some time
}

void UOAnim::setCachePointers(UOHues* hues)
{
    m_UOAnimMUL.m_UOHues = hues;
    m_UOAnimUOP.m_UOHues = hues;
}

QImage* UOAnim::drawAnimFrame(int bodyID, int action, int direction, int frame, unsigned int hueIndex)
{
    if (m_UOAnimUOP.animExists(bodyID))
        return m_UOAnimUOP.drawAnimFrame(bodyID, action, direction, frame, hueIndex);
    else
        return m_UOAnimMUL.drawAnimFrame(bodyID, action, direction, frame, hueIndex);
}


}
