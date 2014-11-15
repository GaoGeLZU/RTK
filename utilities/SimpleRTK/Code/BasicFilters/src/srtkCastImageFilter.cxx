/*=========================================================================
 *
 *  Copyright Insight Software Consortium & RTK Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#include "srtkCastImageFilter.h"


namespace rtk
{
namespace simple
{


//----------------------------------------------------------------------------

//
// Default constructor that initializes parameters
//
CastImageFilter::CastImageFilter ()
{
  this->m_OutputPixelType = srtkFloat32;

  m_DualMemberFactory.reset( new  detail::DualMemberFunctionFactory<MemberFunctionType>( this ) );

  this->RegisterMemberFactory2();
  this->RegisterMemberFactory2v();
  this->RegisterMemberFactory2l();
  this->RegisterMemberFactory3();
  this->RegisterMemberFactory3v();
  this->RegisterMemberFactory3l();


}

//
// ToString
//
std::string CastImageFilter::ToString() const
{
  std::ostringstream out;
  out << "rtk::simple::CastImageFilter\n"
      << "\tOutputPixelType: " << this->m_OutputPixelType << std::endl;
  return out.str();
}

//
// Set/Get Methods for output pixel type
//

CastImageFilter::Self& CastImageFilter::SetOutputPixelType( PixelIDValueEnum pixelID )
{
  this->m_OutputPixelType = pixelID;
  return *this;
}


CastImageFilter::Self& CastImageFilter::SetOutputPixelType( PixelIDValueType pixelID )
{
  return this->SetOutputPixelType( static_cast<PixelIDValueEnum>(pixelID) );
}

PixelIDValueEnum CastImageFilter::GetOutputPixelType( void ) const
{
  return this->m_OutputPixelType;
}


//
// Execute
//
Image CastImageFilter::Execute ( const Image& image )
{

  const PixelIDValueEnum inputType = image.GetPixelID();
  const PixelIDValueEnum outputType = this->m_OutputPixelType;
  const unsigned int dimension = image.GetDimension();

  if (this->m_DualMemberFactory->HasMemberFunction( inputType, outputType,  dimension ) )
    {
    return this->m_DualMemberFactory->GetMemberFunction( inputType, outputType, dimension )( image );
    }

  srtkExceptionMacro( << "Filter does not support casting from casting " << rtk::simple::GetPixelIDValueAsString (inputType) << " to "
                      << rtk::simple::GetPixelIDValueAsString (outputType) );

}


//----------------------------------------------------------------------------


Image Cast ( const Image& image, PixelIDValueEnum pixelID ) {
  CastImageFilter filter;
  return filter.SetOutputPixelType ( pixelID ).Execute ( image );
}


} // end namespace simple
} // end namespace rtk
