#ifndef THNN_OMP_OVERHEAD_THRESHOLD
#define THNN_OMP_OVERHEAD_THRESHOLD 10
#endif
#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/DistKLDivCriterion.c"
#else

void THNN_(DistKLDivCriterion_updateOutput)(
          THNNState *state,
          THTensor *input,
          THTensor *target,
          THTensor *output,
          bool sizeAverage,
          bool reduce)
{
  THNN_CHECK_NELEMENT(input, target);

  if (!reduce) {
    THTensor_(resizeAs)(output, input);
    int serial_path = 0;
#ifdef _OPENMP
    int inOMP = omp_in_parallel();
    if (inOMP) {
      serial_path = 1;
    } else {
      int64_t input_size = THTensor_(nElement)(input);
      int input_contig = THTensor_(isContiguous)(input);
      int target_contig = THTensor_(isContiguous)(target);
      int output_contig = THTensor_(isContiguous)(output);
      TH_TENSOR_APPLY3_OMP(input_size, input_contig, target_contig, output_contig, 
        real, input, real, target, real, output,
        *output_data = *target_data > 0 ? *target_data * (log(*target_data) - *input_data) : 0;,
        THNN_OMP_OVERHEAD_THRESHOLD
      );
    }
#else
    serial_path = 1;
#endif
    if (serial_path) {
      TH_TENSOR_APPLY3(real, input, real, target, real, output,
        *output_data = *target_data > 0 ? *target_data * (log(*target_data) - *input_data) : 0;
      );
    }
    return;
  }

  THTensor_(resize1d)(output, 1);

  real sum = 0;

  TH_TENSOR_APPLY2(real, input, real, target,
    sum += *target_data > 0 ? *target_data * (log(*target_data) - *input_data) : 0;
  );
  if (sizeAverage)
    sum /= THTensor_(nElement)(input);

  THTensor_(set1d)(output, 0, sum);
}

void THNN_(DistKLDivCriterion_updateGradInput)(
          THNNState *state,
          THTensor *input,
          THTensor *target,
          THTensor *gradOutput,
          THTensor *gradInput,
          bool sizeAverage,
          bool reduce)
{
  THNN_CHECK_NELEMENT(input, target);
  THTensor_(resizeAs)(gradInput, input);

  if (!reduce) {
    THNN_CHECK_NELEMENT(input, gradOutput);
    int serial_path = 0;
#ifdef _OPENMP
    int inOMP = omp_in_parallel();
    if (inOMP) {
      serial_path = 1;
    } else {
      int64_t gradInput_size = THTensor_(nElement)(gradInput);
      int gradInput_contig = THTensor_(isContiguous)(gradInput);
      int gradOutput_contig = THTensor_(isContiguous)(gradOutput);
      int target_contig = THTensor_(isContiguous)(target);
      TH_TENSOR_APPLY3_OMP(gradInput_size, gradInput_contig, gradOutput_contig, target_contig,
        real, gradInput, real, gradOutput, real, target,
        *gradInput_data = *target_data > 0 ? (-*target_data) * *gradOutput_data : 0;,
        THNN_OMP_OVERHEAD_THRESHOLD
      );
    }
#else
    serial_path = 1;
#endif
    if (serial_path) {
      TH_TENSOR_APPLY3(real, gradInput, real, gradOutput, real, target,
        *gradInput_data = *target_data > 0 ? (-*target_data) * *gradOutput_data : 0;
      );
    }
    return;
  }

  THNN_CHECK_DIM_SIZE(gradOutput, 1, 0, 1);

  real norm = (sizeAverage ? 1./((real)THTensor_(nElement)(input)) : 1.);

  int serial_path = 0;
#ifdef _OPENMP
  int inOMP = omp_in_parallel();
  if (inOMP) {
    serial_path = 1;
  } else {
    int64_t gradInput_size = THTensor_(nElement)(gradInput);
    int gradInput_contig = THTensor_(isContiguous)(gradInput);
    int input_contig = THTensor_(isContiguous)(input);
    int target_contig = THTensor_(isContiguous)(target);
    TH_TENSOR_APPLY3_OMP(gradInput_size, gradInput_contig, input_contig, target_contig, 
      real, gradInput, real, input, real, target,
      *gradInput_data = *target_data > 0 ? norm * (-*target_data) * THTensor_fastGet1d(gradOutput, 0) : 0;,
      THNN_OMP_OVERHEAD_THRESHOLD
    );
  }
#else
  serial_path = 1;
#endif
  if (serial_path) {
    TH_TENSOR_APPLY3(real, gradInput, real, input, real, target,
      *gradInput_data = *target_data > 0 ? norm * (-*target_data) * THTensor_fastGet1d(gradOutput, 0) : 0;
    );
  }
}

#undef THNN_OMP_OVERHEAD_THRESHOLD
#endif
