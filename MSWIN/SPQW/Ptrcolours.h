// Ptrcolours.h: interface for the CPtrcolours class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PTRCOLOURS_H__F67ACEC4_4692_11D4_9542_00E09872E940__INCLUDED_)
#define AFX_PTRCOLOURS_H__F67ACEC4_4692_11D4_9542_00E09872E940__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPtrcolours  
{
private:
	COLORREF	m_table[8];

public:
	CPtrcolours();
	~CPtrcolours();

	enum pcl_type	{
		IDLE_CL = 0, PRINTING_CL = 1, AWOPER_CL = 2, HALTED_CL = 3,
		ERROR_CL = 4, OFFLINE_CL = 5, STARTUP_CL = 6, SHUTD_CL = 7 };

	COLORREF & operator [] (const unsigned);

private:
	const pcl_type whichcol(const unsigned);

};

#endif // !defined(AFX_PTRCOLOURS_H__F67ACEC4_4692_11D4_9542_00E09872E940__INCLUDED_)
