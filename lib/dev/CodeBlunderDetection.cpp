#pragma ident "$Id$"

/**
 * @file CodeBlunderDetection.cpp
 * This is a class to detect code blunders using quality control theory
 * 
 * For more info, please refer to: 
 * Banville and B. Langley (2013) Mitigating the impact of ionospheric cycle 
 * slips in GNSS observations 
 *
 */  


//============================================================================
//
//  This file is part of GPSTk, the GPS Toolkit.
//
//  The GPSTk is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published
//  by the Free Software Foundation; either version 2.1 of the License, or
//  any later version.
//
//  The GPSTk is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with GPSTk; if not, write to the Free Software Foundation,
//  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
//
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2007, 2008, 2011
//
//============================================================================
// Date     :	2017/08/15 - 2017/08/16
// Version	:	0.0
//	Author(s):	Lei Zhao, a Ph.D. candiate
// School of Geodesy and Geomatics, Wuhan University
//============================================================================

#include "CodeBlunderDetection.hpp"

namespace gpstk
{
		// Return a string identifying this object
	std::string CodeBlunderDetection::getClassName() const
	{ return "CodeBlunderDetection"; }




      /* Returns a gnnsRinex object, adding the new data generated when
       * calling this object.
       *
       * @param gData    Data object holding the data.
       */
   gnssRinex& CodeBlunderDetection::Process(gnssRinex& gData)
      throw(ProcessingException)
   {

      try
      {

			Process(gData.header.epoch, gData.body, gData.header.epochFlag);			

         return gData;

      }
      catch(Exception& u)
      {
            // Throw an exception if something unexpected happens
         ProcessingException e( getClassName() + ":"
                                + u.what() );

         GPSTK_THROW(e);

      }

   }  // End of method 'CycleSlipEstimator::Process()'



      /** Returns a satTypeValueMap object, adding the new data generated
       *  when calling this object.
       *
       * @param epoch     Time of observations.
       * @param gData     Data object holding the data.
       * @param epochflag Epoch flag.
       */
	satTypeValueMap& CodeBlunderDetection::Process( const CommonTime& epoch,
																	satTypeValueMap& gData,
																	const short& epochflag )
     throw(ProcessingException)
	{
 		try
 		{
			// debug code vvv
			CivilTime ct(epoch);
			std::cout << ct.year << " " << ct.month << " " << ct.day 
							<< " " << ct.hour << " " << ct.minute << " " 
								<< ct.second << std::endl;
			// debug code ^^^ 

 			SatIDSet satRejectedSet;

				// clear sat time-differenced LI data
				// at the beginning of each processing epoch 
			satLITimeDiffData.clear();
 
 				// Loop through all the satellites
 			satTypeValueMap::iterator it;
 			for( it = gData.begin(); it != gData.end(); ++it )
 			{
 				
 				
 					// SatID 
 				SatID sat( it -> first );
				typeValueMap tvm( it -> second );
				typeValueMap codeData;

					// Check required code data 
				double testValue(0.0);
				try
				{

						// Loop through the codeTypes to get their values
					for( TypeIDSet::const_iterator it = codeTypes.begin();
						  it != codeTypes.end();
						  ++it )
					{
							// Present type	
						TypeID type( *it );

							// Try to get the value of this sat 
						testValue = tvm( type );

					}   // End of ' for( TypeIDSet::const_iterator it ... '
	

				}
				catch(...)
				{
						// If some value is missing, then schedule this satellite
						// for removal
					satRejectedSet.insert( sat );
					continue;

				}   // End of 'try-catch' block

				//  debug code vvv
				std::cout << sat <<  std::endl;
				//  debug code ^^^ 

 					// Get filter data of this sat 
				bool isTimeContinuous(true);
 				isTimeContinuous = !getSatFilterData( epoch, sat, 
																  tvm,	epochflag ); 

 			}   // End of ' for( it = gData.begin();  ... '

				// Remove satellites with missing data
			gData.removeSatID(satRejectedSet);


				// Now, let's model time-differenced code observations 
			size_t numOfSats( satCodeTimeDiffData.numSats() );
			if( numOfSats >= 4 )
			{
//				timeDiffCodeModel();
				std::cout << "Do estimation!!!" << std::endl;
			}   
			else
			{
				std::cout << "not enough sats" << std::endl;
			}


				// Clear satCodeTimeDiffData, which is generated at every epoch 
			satCodeTimeDiffData.clear();


		
			return gData;
			

//			// debug code vvv
//			
//			SatID sat1(1, SatID::systemGPS );
//			CommonTime time( satFormerData( sat1 ).epoch );
//			std::cout << "Test C1 value: " << satFormerData( sat1 )( TypeID::C1) << std::endl;
//		
//			// debug code ^^^ 
 
 		}
 
 		catch( Exception& u )
 		{
 				// Thrown an exception if something unexpected happens
 			ProcessingException e( getClassName() + ":" 
 											+ u.what() );
 
 			GPSTK_THROW(e);
 
 		}   // End of 'try-catch' block
 
 
 
 	}   // End of 'satTypeValueMap& CodeBlunderDetection::Process( ... '



		/* Get filter data
		 *
		 * Return the flag indicating whether the phase is continious in time
		 * and there are LLI cycle-slip declaration(TO DO!!!) 
		 *
		 * @param epoch
		 * @param sat
		 * @param tvMap      data related to this sat
		 * @param epochFlag 
		 *
		 */
	bool CodeBlunderDetection::getSatFilterData( const CommonTime& epoch, 
																  const SatID& sat, 
																  typeValueMap& tvMap,
																  const short& epochFlag )
	{

		bool reportTimeInteruption(false);



			// Difference between current and former epochs, in sec
		double currentDeltaT(0.0);

			// Incorporate epoch and tvMap  
		epochTypeValueBody etvb(epoch, tvMap);
		
			//  Is this  a new visible sat?
		satEpochTypeValueMap::const_iterator itFormData = satFormerData.find( sat );
		if( itFormData == satFormerData.end() )
		{
				// This means a new sat
			satFormerData[sat] = etvb;
			//satFormerData[sat].epoch = epoch;

				// Just return
			reportTimeInteruption = true;; 
			return reportTimeInteruption;
		
		}   // End of ' if( itFormData == satFormerData.end() ) '


			// Get the difference between current epoch and former epoch,
			// in seconds
		currentDeltaT = ( epoch - satFormerData[sat].epoch );
		
		if( currentDeltaT > deltaTMax )
		{
				// This means time gap of this sat, report time interuption
			reportTimeInteruption = true;

				// Updata satFormerData 
			satFormerData[sat] = etvb;

				// Just return
			return reportTimeInteruption;

		}
		
			// TO DO!!!
			// epochFlag 

			// Now, everything is OK!
			// Loop through the codeTypes to compute time-differenced values
		for( TypeIDSet::const_iterator it = codeTypes.begin();
			  it != codeTypes.end();
			  ++it )
		{
			TypeID type( *it );

			
				// Time differenced value of this type 
			satCodeTimeDiffData[sat][type] = tvMap(type) - 
														satFormerData(sat)(type);

		
		}   // End of ' for( TypeIDSet::const_iterator it = obsTypes.begin(); ... '

		
			// Loop through the liTypes to compute time-differenced values
		if( useExternalIonoDelayInfo )
		{
			for( TypeIDSet::const_iterator it = liTypes.begin(); 
				  it != liTypes.end(); 
				  ++it )
			{
				TypeID type( *it ), outType( *it );

				if( type == TypeID::LI )
				{
					outType = TypeID::deltaLI;
				}

					// Time differenced value of this type 
				double deltaLI(0.0);
				deltaLI = tvMap(type) - satFormerData(sat)(type);

				satLITimeDiffData[sat][outType] = deltaLI;


					// Record deltaLI in etvb
				etvb[outType] = deltaLI;

					// Double time-differenced LI 
				double deltaDeltaLI(0.0);
				epochTypeValueBody::const_iterator iter = 
												satFormerData(sat).find( TypeID::deltaLI );	
				if( iter != satFormerData(sat).end() )
				{
					deltaDeltaLI = deltaLI - satFormerData(sat)(outType);
					satLITimeDiffData[sat][ TypeID::deltaDeltaLI ] = deltaDeltaLI;
				}
				
			}   // End of ' for( TypeIDSet::const_iterator it =  ... '

		}   // End of ' if( useExternalIonoDelayInfo ) '


			// Update satFormerData data
		satFormerData[sat] = etvb;

		return reportTimeInteruption;


	}   // End of 'void CycleSlipEstimator::getFilterData( const SatID& sat, ... '




	

}   // End of namespace