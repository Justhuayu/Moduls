#include "MultiPushStream.h"

MultiPushStream::MultiPushStream()
{
    m_dataQueue = std::make_shared<ThreadSafeCircularQueue<InputData>>(m_dataCapacity);
}
