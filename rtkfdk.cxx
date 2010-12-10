#include "rtkfdk_ggo.h"
#include "rtkGgoFunctions.h"
#include "rtkConfiguration.h"

#include "itkThreeDCircularProjectionGeometryXMLFile.h"
#include "itkProjectionsReader.h"
#include "itkFFTRampImageFilter.h"
#include "itkFDKWeightProjectionFilter.h"

#include "itkFDKBackProjectionImageFilter.h"
#if CUDA_FOUND
#  include "itkCudaFDKBackProjectionImageFilter.h"
#endif

#include <itkImageFileWriter.h>
#include <itkRegularExpressionSeriesFileNames.h>
#include <itkTimeProbe.h>
#include <itkStreamingImageFilter.h>

int main(int argc, char * argv[])
{
  GGO(rtkfdk, args_info);

  typedef float OutputPixelType;
  const unsigned int Dimension = 3;

  typedef itk::Image< OutputPixelType, Dimension > OutputImageType;

  // Generate file names
  itk::RegularExpressionSeriesFileNames::Pointer names = itk::RegularExpressionSeriesFileNames::New();
  names->SetDirectory(args_info.path_arg);
  names->SetNumericSort(false);
  names->SetRegularExpression(args_info.regexp_arg);
  names->SetSubMatch(0);

  if(args_info.verbose_flag)
    std::cout << "Regular expression matches "
              << names->GetFileNames().size()
              << " file(s)..."
              << std::endl;

  // Projections reader
  typedef itk::ProjectionsReader< OutputImageType > ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileNames( names->GetFileNames() );
  reader->GenerateOutputInformation();

  itk::TimeProbe readerProbe;
  try {
    readerProbe.Start();
    reader->Update();
    readerProbe.Stop();
  } catch( itk::ExceptionObject & err ) {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
  }

  if(args_info.verbose_flag)
    std::cout << "Reading geometry information from "
              << args_info.geometry_arg
              << "..."
              << std::endl;

  // Geometry
  itk::ThreeDCircularProjectionGeometryXMLFileReader::Pointer geometryReader = itk::ThreeDCircularProjectionGeometryXMLFileReader::New();
  geometryReader->SetFilename(args_info.geometry_arg);
  geometryReader->GenerateOutputInformation();

  // Weight projections according to fdk algorithm
  typedef itk::FDKWeightProjectionFilter< OutputImageType > WeightFilterType;
  WeightFilterType::Pointer weightFilter = WeightFilterType::New();
  weightFilter->SetInput( reader->GetOutput() );
  weightFilter->SetSourceToDetectorDistance( geometryReader->GetOutputObject()->GetSourceToDetectorDistance() );

  // Ramp filter
  typedef itk::FFTRampImageFilter<OutputImageType> RampFilterType;
  RampFilterType::Pointer rampFilter = RampFilterType::New();
  rampFilter->SetInput( weightFilter->GetOutput() );
  rampFilter->SetTruncationCorrection(args_info.pad_arg);

  // Streaming filter
  typedef itk::StreamingImageFilter<OutputImageType, OutputImageType> StreamerType;
  StreamerType::Pointer streamer = StreamerType::New();
  streamer->SetInput( rampFilter->GetOutput() );
  streamer->SetNumberOfStreamDivisions( 1 + reader->GetOutput()->GetLargestPossibleRegion().GetNumberOfPixels() / (1024*1024*4) );

  if(args_info.verbose_flag)
    std::cout << "Weighting and filtering projections..."
              << std::endl;

  // Try to do all 2D pre-processing
  itk::TimeProbe streamerProbe;
  try {
    streamerProbe.Start();
    streamer->Update();
    streamerProbe.Stop();
  } catch( itk::ExceptionObject & err ) {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
  }

  // Create reconstructed volume
  OutputImageType::Pointer tomography = rtk::CreateImageFromGgo<OutputImageType>(args_info);

  if(args_info.verbose_flag)
    std::cout << "Backprojecting using "
              << args_info.hardware_arg
              << std::endl;

  // Backprojection
  typedef itk::FDKBackProjectionImageFilter<OutputImageType, OutputImageType> BackProjectionFilterType;
  BackProjectionFilterType::Pointer bpFilter;
  if(!strcmp(args_info.hardware_arg, "cpu"))
    bpFilter = BackProjectionFilterType::New();
  else if(!strcmp(args_info.hardware_arg, "cuda"))
    {
#if CUDA_FOUND
    bpFilter = itk::CudaFDKBackProjectionImageFilter::New();
#else
    std::cerr << "The program has not been compiled with cuda option" << std::endl;
    return EXIT_FAILURE;
#endif
    }
  bpFilter->SetInput( 0, tomography );
  bpFilter->SetInput( 1, streamer->GetOutput() );
  bpFilter->SetGeometry( geometryReader->GetOutputObject() );
  bpFilter->SetInPlace( true );

  itk::TimeProbe bpProbe;
  try {
    bpProbe.Start();
    bpFilter->Update();
    bpProbe.Stop();
  } catch( itk::ExceptionObject & err ) {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
  }

  // Write
  typedef itk::ImageFileWriter<  OutputImageType >  WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( args_info.output_arg );
  writer->SetInput( bpFilter->GetOutput() );

  itk::TimeProbe writerProbe;
  try {
    writerProbe.Start();
    writer->Update();
    writerProbe.Stop();
  } catch( itk::ExceptionObject & err ) {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
  }

  if(args_info.verbose_flag)
    std::cout << "Reading took " << readerProbe.GetMeanTime() << ' ' << readerProbe.GetUnit() << std::endl
              << "2D processing took " << streamerProbe.GetMeanTime() << ' ' << streamerProbe.GetUnit() << std::endl
              << "backprojection took " << bpProbe.GetMeanTime() << ' ' << bpProbe.GetUnit() << std::endl
              << "writing took " << writerProbe.GetMeanTime() << ' ' << writerProbe.GetUnit() << std::endl;

  return EXIT_SUCCESS;
}
