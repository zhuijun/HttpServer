
#pragma once

class BaseBuffer	 
{
private:
	BaseBuffer( const BaseBuffer& );
	BaseBuffer& operator=( const BaseBuffer& ); 

protected:

	char*	m_pBuffer;
	unsigned int	m_uiBufferSize,	m_uiHead, m_uiTail;

protected:

	char* Head() const;
	
public:

	operator char*();

	void Empty();

	bool IsEmpty() const;

	void Added(unsigned int);

	void Removed(unsigned int);

	bool Add( const char*, unsigned int);

	void MoveData();

	char* Data() const { return Head(); }

	unsigned int DataSize() const;

	unsigned int FreeSize() const;

	char* Tail() const;

	unsigned int BufferSize() const;

    char* Buffer() const;

	void InitBuffer(char* pBuffer, unsigned int uiSize);

	BaseBuffer( char* pBuffer = nullptr, unsigned int uiSize = 0 );

	virtual ~BaseBuffer();
};


