#ifndef __CWM_EVENT_STRUCTURE_H__
#define __CWM_EVENT_STRUCTURE_H__

/**
  * @brief  CyweeMotion task handle event structure. 
  */
typedef struct {
    uint32_t tid;
    uint32_t handleType;
    int (*handleEvent)(void *pHandle, uint32_t evtType, void* evtData);
    void *mem;
} CWMHandle_t, *pCWMHandle_t;


/**
  * @brief  Sensor event structure. 
  */
typedef struct {
    uint32_t sensorType;
    uint32_t index;
    float fData[16];
    uint64_t timestamp_ns;
    void *memData;
} SensorEVT_t, *pSensorEVT_t;

#endif /* __CWM_EVENT_STRUCTURE_H__ */
