Use Ethernet.payload, then IPv4.header; neither layer always has its minimum length.
===
Handle fragments before transport parsing and pass only total-header bytes to the transport helper.
