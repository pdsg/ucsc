#if !defined BKS_DEVICECONTEXT_H
#define BKS_DEVICECONTEXT_H


// -------------------------------------------------------------------------
struct BksDeviceContext {
	int nr;
	struct BksBuffer* buf;
	struct BksRingBuffer* ringbuf;
};



#endif // !defined BKS_DEVICECONTEXT_H
