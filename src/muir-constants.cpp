//
// C++ Implementation: muir-constants
//
// Description: Constants for MUIR experiment data.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
// 
//
//

#include "muir-constants.h"

// Location Constants
const std::string RTI_RAWPULSEWIDTH_PATH("/Raw11/Data/Pulsewidth");
const std::string RTI_RAWTXBAUD_PATH("/Raw11/Data/TxBaud");
const std::string RTI_RAWSAMPLEDATA_PATH("/Raw11/Data/Samples/Data");
const std::string RTI_RAWSAMPLERANGE_PATH("/Raw11/Data/Samples/Range");
const std::string RTI_RAWFRAMECOUNT_PATH("/Raw11/Data/RadacHeader/FrameCount");
const std::string RTI_EXPERIMENTFILE_PATH("/Setup/Experimentfile");
const std::string RTI_RADACTIME_PATH("/Time/RadacTime");


const std::string RTI_DECODEDDIR_PATH("/Decoded");
const std::string RTI_DECODEDDATA_PATH ("/Decoded/Data");
const std::string RTI_DECODEDRANGE_PATH("/Decoded/Range");
const std::string RTI_DECODEDRADAC_PATH("/Decoded/RadacTime");
const std::string RTI_DECODEDFRAME_PATH("/Decoded/FrameCount");

const std::string RTI_DECODEDFFTSIZE_PATH("/Decoded/FFTSize");
const std::string RTI_DECODEDTIMEINTEGRATION_PATH("/Decoded/TimeIntegration");
const std::string RTI_DECODEDPHASECODEMUTING_PATH("/Decoded/PhasecodeMuting");
const std::string RTI_DECODEDDECODINGTHREADS_PATH("/Decoded/DecodingThreads");
const std::string RTI_DECODEDDECODINGPLATFORM_PATH("/Decoded/DecodingPlatform");
const std::string RTI_DECODEDDECODINGPROCESS_PATH("/Decoded/DecodingProcess");
const std::string RTI_DECODEDDECODINGTIME_PATH("/Decoded/DecodingTime");
const std::string RTI_DECODEDSOURCEFILE_PATH("/Decoded/SourceFile");

const std::string RTI_DECODEDROWTIMINGDIR_PATH("/Decoded/RowTiming");
const std::string RTI_DECODEDROWTIMINGDATA_PATH("/Decoded/RowTiming/Data");
const std::string RTI_DECODEDROWTIMINGCOLUMNS_PATH("/Decoded/RowTiming/Columns");