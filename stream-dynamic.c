#include <stdio.h>
#include <windows.h>
#include <gestic_api.h>

#define thresholdLevel 30
#define preBufferSize 100
#define eventRecordingMaxSize 1000

typedef struct CSVLine {
    int SD_CH_0;
    int SD_CH_1;
    int SD_CH_2;
    int SD_CH_3;
    int SD_CH_4;
    int CIC_CH_0;
    int CIC_CH_1;
    int CIC_CH_2;
    int CIC_CH_3;
    int CIC_CH_4;
    int gesture;
} CSVLine;

static const char* gestures[] = {
    "-",
    "W>E",
    "E>W",
    "S>N",
    "N>S",
    "CW",
    "CCW"
};

int writeCSV(struct CSVLine* data, char* fileName)
{
    errno_t err;
    FILE* stream;
    err = fopen_s(&stream, fileName, "a+");
    if (err != 0) return -1;
    fprintf(stream, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", data->CIC_CH_0, data->CIC_CH_1, data->CIC_CH_2, data->CIC_CH_3, data->CIC_CH_4, data->SD_CH_0, data->SD_CH_1, data->SD_CH_2, data->SD_CH_3, data->SD_CH_4, data->gesture);
    err = fclose(stream);
    return 0;
}

int writeFullCSV(CSVLine* preData, CSVLine* mainData, int preBufferIndex, int mainBufferIndex, char* fileName) {
    int i;
    errno_t err;
    FILE* stream;
    err = fopen_s(&stream, fileName, "a+");
    if (err != 0) return -1;
    for (i = 0; i < preBufferSize; i++) {
        fprintf(stream, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", preData[preBufferIndex].CIC_CH_0, preData[preBufferIndex].CIC_CH_1, preData[preBufferIndex].CIC_CH_2, preData[preBufferIndex].CIC_CH_3, preData[preBufferIndex].CIC_CH_4, preData[preBufferIndex].SD_CH_0, preData[preBufferIndex].SD_CH_1, preData[preBufferIndex].SD_CH_2, preData[preBufferIndex].SD_CH_3, preData[preBufferIndex].SD_CH_4, preData[preBufferIndex].gesture);
        preBufferIndex++;
        if (preBufferIndex == preBufferSize) preBufferIndex = 0; // rollover
    }
    for (i = 0; i < mainBufferIndex; i++) {
        fprintf(stream, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", mainData[i].CIC_CH_0, mainData[i].CIC_CH_1, mainData[i].CIC_CH_2, mainData[i].CIC_CH_3, mainData[i].CIC_CH_4, mainData[i].SD_CH_0, mainData[i].SD_CH_1, mainData[i].SD_CH_2, mainData[i].SD_CH_3, mainData[i].SD_CH_4, mainData[i].gesture);
    }

    err = fclose(stream);
    return 0;
}

int main() {
    int i,n;
    CSVLine *data_row;
    CSVLine *preBuffer;
    unsigned int preBufferIndex = 0;
    unsigned int preBufferFullFlag = 0;
    CSVLine *mainBuffer;
    unsigned int mainBufferIndex = 0;
    unsigned int mainBufferIndexEnd = 0;
    unsigned int stateMachine = 0;

    char filename[10];

    data_row = (CSVLine*)malloc(sizeof(CSVLine));
    preBuffer = (CSVLine*)malloc(sizeof(CSVLine)*(preBufferSize+1));
    mainBuffer = (CSVLine*)malloc(sizeof(CSVLine)*(eventRecordingMaxSize+1));

    /* Create GestIC-Instance */
    gestic_t *gestic = gestic_create();

    /* Bitmask later used for starting a stream with SD- and position-data */
    const int stream_flags = gestic_data_mask_sd | gestic_data_mask_dsp_status | gestic_data_mask_gesture | gestic_data_mask_cic | gestic_data_mask_all;

    /* Pointer to the internal data-buffers */
    const gestic_signal_t* sd = NULL;
    const gestic_gesture_t * gest = NULL;
    const gestic_signal_t * cic = NULL;

    /* Initialize all variables and required resources of gestic */
    gestic_initialize(gestic);

    /* Aquire the pointers to the data-buffers */
    sd = gestic_get_sd(gestic, 0);
    cic = gestic_get_cic(gestic, 0);
    gest = gestic_get_gesture(gestic, 0);

    /* Try to open a connection to the device */
    if(gestic_open(gestic) < 0) {
        fprintf(stderr, "Could not open connection to device.\n");
        return -1;
    }

    /* Try to reset the device to the default state:
     * - Automatic calibration enabled
     * - All frequencies allowed
     * - Approach detection disabled
     */
    if(gestic_set_auto_calibration(gestic, 1, 100) < 0 ||
       gestic_select_frequencies(gestic, gestic_all_freq, 100) < 0 ||
       gestic_set_approach_detection(gestic, 0, 100) < 0 ||
       gestic_set_enabled_gestures(gestic, 126, 100) < 0)
    {
        fprintf(stderr, "Could not reset device to default state.\n");
        return -1;
    }

    /* Set output-mask to the bitmask defined above and stream all data */
    if(gestic_set_output_enable_mask(gestic, stream_flags, 0,
                                     gestic_data_mask_all, 100) < 0)
    {
        fprintf(stderr, "Could not set output-mask for streaming.\n");
        return -1;
    }
    for (n = 0; n < 50; n++) {
        printf("Please wait ... ");
        for (i = 0; i < 40; ++i) {
            stateMachine = 0;
            mainBufferIndex = 0;
            mainBufferIndexEnd = 0;
            preBufferIndex = 0;
            preBufferFullFlag = 0;
            gestic_data_stream_update(gestic, 0);
        }
        while (stateMachine != 2) {
            while (!gestic_data_stream_update(gestic, 0)) {
                // Fill data to SCV struct
                data_row->SD_CH_0 = sd->channel[0];
                data_row->SD_CH_1 = sd->channel[1];
                data_row->SD_CH_2 = sd->channel[2];
                data_row->SD_CH_3 = sd->channel[3];
                data_row->SD_CH_4 = sd->channel[4];
                data_row->CIC_CH_0 = cic->channel[0];
                data_row->CIC_CH_1 = cic->channel[1];
                data_row->CIC_CH_2 = cic->channel[2];
                data_row->CIC_CH_3 = cic->channel[3];
                data_row->CIC_CH_4 = cic->channel[4];
                data_row->gesture = gest->gesture;

                if (stateMachine == 0) {
                    if ((sd->channel[0] > thresholdLevel) || (sd->channel[1] > thresholdLevel) || (sd->channel[2] > thresholdLevel) || (sd->channel[3] > thresholdLevel) || (sd->channel[4] > thresholdLevel)) {
                        //start saving to main buffer - event started
                        stateMachine = 1;
                        printf("Gesture detected! Recording ... ");
                    }
                    else {
                        memcpy(&preBuffer[preBufferIndex], data_row, sizeof(CSVLine));
                        preBufferIndex++;
                        if (preBufferIndex > preBufferSize) {
                            preBufferFullFlag++;
                            preBufferIndex = 0;
                            if (preBufferFullFlag == 1) {
                                printf("System ready!\n");
                            }
                        }
                    }
                }
                if (stateMachine == 1) {
                    if ((sd->channel[0] > thresholdLevel) || (sd->channel[1] > thresholdLevel) || (sd->channel[2] > thresholdLevel) || (sd->channel[3] > thresholdLevel) || (sd->channel[4] > thresholdLevel)) {
                        mainBufferIndexEnd = 0;
                    }
                    else {
                        mainBufferIndexEnd++;
                    }
                    memcpy(&mainBuffer[mainBufferIndex], data_row, sizeof(CSVLine));
                    mainBufferIndex++;
                    if ((mainBufferIndex > eventRecordingMaxSize) || (mainBufferIndexEnd > preBufferSize)) {
                        printf("OK\n");
                        stateMachine = 2;
                        sprintf(filename, "%d.csv", n);
                        writeFullCSV(preBuffer, mainBuffer, preBufferIndex, mainBufferIndex, filename);
                    }
                }
            }
        }
    }

    /* Close connection to device */
    gestic_close(gestic);

    /* Release further resources that were used by gestic */
    gestic_cleanup(gestic);
    gestic_free(gestic);

    free(data_row);
    free(preBuffer);
    free(mainBuffer);

    return 0;
}