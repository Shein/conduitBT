/*
  Module Name:
    Queue.h

  Abstract:
    Queue events for bth HFP

  Environment:
    Kernel mode only
*/



/*
 This routine is invoked by the framework to deliver a Read request to the driver.

 Arguments:
    Queue - Queue delivering the request
    Request - Read request
    Length - Length of Read
*/
EVT_WDF_IO_QUEUE_IO_READ  HfpEvtQueueIoRead;


/*
 This routine is invoked by the framework to deliver a write request to the driver.

 Arguments:
    Queue - Queue delivering the request
    Request - Write request
    Length - Length of write
*/
EVT_WDF_IO_QUEUE_IO_WRITE  HfpEvtQueueIoWrite;


/*
 This routine is invoked by Framework when Queue is being stopped
 We implement this routine to cancel any requests owned by our driver.
 Without this Queue stop would wait indefinitely for requests to complete during surprise remove.

Arguments:
    Queue - Framework queue being stopped
    Request - Request owned by the driver
    ActionFlags - Action flags
*/
EVT_WDF_IO_QUEUE_IO_STOP  HfpEvtQueueIoStop;


/*
 This routine is invoked by the framework to deliver a DeviceIoControl request.

 Arguments:
    Queue - Queue delivering the request
    Request - Read request
    OutputBufferLength,
    InputBufferLength,
    IoControlCode
*/
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL  HfpEvtQueueIoDeviceControl;


/*
 Completion routine for read/write requests.
 We receive a transfer BRB as the context. This BRB is part of the request context and doesn't need 
 to be freed explicitly.
    
Arguments:
    Request - Request that got completed
    Target  - Target to which request was sent
    Params  - Completion parameters for the request
    Context - We receive BRB as the context
*/
EVT_WDF_REQUEST_COMPLETION_ROUTINE	HfpReadWriteCompletion;
