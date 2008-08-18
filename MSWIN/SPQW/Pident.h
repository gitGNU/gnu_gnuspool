class	pident	{
  private:
	netid_t		remote_id;		// Host it is on (0 means this one)
	slotno_t	remote_slot;	// Slot number in printer queue
  public:	
	pident(netid_t h = 0, slotno_t s = -1) : remote_id(h), remote_slot(s) { }
	pident(const spptr *p)
	{
		remote_id = p->spp_netid;
		remote_slot = remote_id? p->spp_rslot: -1;
	}
	friend  class  plist;
};
