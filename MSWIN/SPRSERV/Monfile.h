// Structure for remembering file names to monitor

#define	MON_INTERVAL	5000
#define	INIT_LIST	10
#define	INC_LIST	5

class	CMonFile : public CObject	{
private:
	BOOL		mon_notyet;
	unsigned	mon_time;
	char		mon_filename[_MAX_PATH];
	spq			qparams;
	pages		delimdescr;
	char		*delimiter;
protected:
	CMonFile() {}			//  For serialising
	DECLARE_SERIAL(CMonFile)
public:
	CMonFile(const char *, const spq &, const pages &, const char *, const unsigned = MON_INTERVAL);
	CMonFile(const char *, const CMonFile *, const unsigned = MON_INTERVAL);
	~CMonFile();
	UINT		queuejob(const char *, CFile &);
	char		*get_file() 	{	return  mon_filename;  }
	unsigned	get_time() const	{	return	mon_time;	}
	unsigned	nexttime(CFile &)	const;
	spq		&qpar()		{ return qparams;	}
	pages	&pgs()		{ return delimdescr;	}
	char	*delim()	{ return delimiter;	}
	void	delimset(char *d)	{ if (delimiter) delete [] delimiter; delimiter = d; }
	BOOL	isok() const	{	return  !mon_notyet;	}
	void	setok(BOOL ok = TRUE)	{ mon_notyet = !ok; }
	void	Serialize(CArchive &);
	friend  class	CMFList;
};

class	CMFList	: public CObject	{
private:
	CMonFile	**list;
	unsigned	num, max, lookingat;
public:
	CMFList();
	DECLARE_SERIAL(CMFList)
	~CMFList();
	void	addfile(CMonFile *);
	void	delfile(CMonFile *);
	void	delfile(const unsigned);
	void	repfile(const unsigned, CMonFile *);
	CMonFile	*operator[](const unsigned n) const { return  n < num? list[n]: NULL; }
	void	setfirst() { lookingat = 0; }
	CMonFile	*next();
	unsigned	number() const { return num; }
	void	Serialize(CArchive &);
};
	
	
	
	