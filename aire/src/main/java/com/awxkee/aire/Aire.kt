package com.awxkee.aire

import androidx.annotation.Keep
import com.awxkee.aire.pipeline.BasePipelinesImpl
import com.awxkee.aire.pipeline.BlurPipelinesImpl
import com.awxkee.aire.pipeline.ShiftPipelineImpl

@Keep
object Aire : BlurPipelines by BlurPipelinesImpl(),
    ShiftPipelines by ShiftPipelineImpl(),
    BasePipelines by BasePipelinesImpl() {
    init {
        System.loadLibrary("aire")
    }
}