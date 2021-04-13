
#include "BaseBuffer.h"
#include <string.h>
namespace base
{
    namespace net
    {
        BaseBuffer::BaseBuffer(char* pBuffer, unsigned int uiSize)
            : m_pBuffer(pBuffer), m_uiBufferSize(uiSize), m_uiHead(0), m_uiTail(0)
        {

        }

        BaseBuffer::~BaseBuffer()
        {

        }

        void BaseBuffer::InitBuffer(char* pBuffer, unsigned int uiSize)
        {
            m_pBuffer = pBuffer;
            m_uiBufferSize = uiSize;
            Empty();
        }

        BaseBuffer::operator char*()
        {
            return m_pBuffer;
        }

        bool BaseBuffer::IsEmpty() const
        {
            return (m_uiHead >= m_uiTail);
        }

        void BaseBuffer::Empty()
        {
            m_uiHead = m_uiTail = 0;
            memset(m_pBuffer, 0, m_uiBufferSize);
        }

        void BaseBuffer::Added(unsigned int uiSize)
        {
            m_uiTail += uiSize;
        }

        void BaseBuffer::Removed(unsigned int uiSize)
        {
            if (m_uiHead != m_uiTail)
            {
                m_uiHead += uiSize;
            }
        }

        bool BaseBuffer::Add(const char* pData, unsigned int uiSize)
        {
            unsigned int iTemp = FreeSize();
            //ASSERT( iTemp/*FreeSize()*/ >=uiSize );
            if (iTemp < uiSize)
            {
                return false;
            }
            memcpy(Tail(), pData, uiSize);
            Added(uiSize);
            return true;
        }

        void BaseBuffer::MoveData()
        {
            if (m_uiHead > 0)
            {
                if (false != IsEmpty())
                {
                    m_uiHead = 0;
                    m_uiTail = 0;
                }
                else
                {
                    memmove(Buffer(), Head(), DataSize());
                    m_uiTail -= m_uiHead;
                    m_uiHead = 0;
                }
            }
        }

        char* BaseBuffer::Buffer() const
        {
            return m_pBuffer;
        }

        unsigned int BaseBuffer::BufferSize() const
        {
            return m_uiBufferSize;
        }

        char* BaseBuffer::Head() const
        {
            return (m_pBuffer + m_uiHead);
        }

        char* BaseBuffer::Tail() const
        {
            return (m_pBuffer + m_uiTail);
        }

        unsigned int BaseBuffer::DataSize() const
        {
            return (m_uiTail > m_uiHead) ? (m_uiTail - m_uiHead) : 0;
        }

        unsigned int BaseBuffer::FreeSize() const
        {
            return (BufferSize() > m_uiTail) ? (BufferSize() - m_uiTail) : 0;
        }
    }
}