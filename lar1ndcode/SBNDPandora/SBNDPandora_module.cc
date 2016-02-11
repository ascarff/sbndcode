/**
 *  @file   lar1ndcode/SBNDPandora/SBNDPandora_module.cc
 *
 *  @brief  Producer module for SBND 4APA detector.
 *
 */

#include "art/Framework/Core/ModuleMacros.h"

#include "larcore/Geometry/Geometry.h"

#include "LArStitching/MultiPandoraApi.h"

#include "larpandora/LArPandoraInterface/LArPandora.h"

#include <set>
#include <string>

namespace lar_pandora
{

/**
 *  @brief  SBNDPandora class
 */
class SBNDPandora : public LArPandora
{
public: 
    /**
     *  @brief  Constructor
     *
     *  @param  pset the parameter set
     */
    SBNDPandora(fhicl::ParameterSet const &pset);

    int GetVolumeIdNumber(const unsigned int cryostat, const unsigned int tpc) const;

private:
    void CreatePandoraInstances();

    /**
     *  @brief  Create primary pandora instance
     *
     *  @param  stitchingConfigFileName the pandora settings stitching config file name
     */
    void CreatePrimaryPandoraInstance(const std::string &stitchingConfigFileName);

    /**
     *  @brief  Create daughter pandora instances
     *
     *  @param  theGeometry the geometry handle
     *  @param  configFileName the pandora settings config file name
     */
    void CreateDaughterPandoraInstances(const art::ServiceHandle<geo::Geometry> &theGeometry, const std::string &configFileName);

    typedef std::set<int> IntSet;   ///<

    bool    m_useLeftVolume;        ///<
    bool    m_useRightVolume;       ///<
};

DEFINE_ART_MODULE(SBNDPandora)

} // namespace lar_pandora

//------------------------------------------------------------------------------------------------------------------------------------------
// implementation follows

#include "cetlib/exception.h"

#include "Api/PandoraApi.h"

#include "LArContent.h"

#include "lar1ndcode/SBNDPandora/SBNDPseudoLayerPlugin.h"
#include "lar1ndcode/SBNDPandora/SBNDTransformationPlugin.h"

namespace lar_pandora
{

SBNDPandora::SBNDPandora(fhicl::ParameterSet const &pset) :
    LArPandora(pset)
{
    m_useLeftVolume = pset.get<bool>("UseLeftVolume", true);
    m_useRightVolume = pset.get<bool>("UseRightVolume", true);
}

//------------------------------------------------------------------------------------------------------------------------------------------

int SBNDPandora::GetVolumeIdNumber(const unsigned int cryostat, const unsigned int tpc) const
{
    art::ServiceHandle<geo::Geometry> theGeometry;
    const geo::TPCGeo &theTpcGeo(theGeometry->TPC(tpc, cryostat));

    // Left drift volume: negative drift direction
    if (theTpcGeo.DriftDirection() == geo::kNegX)
    {
        if (m_useLeftVolume)
            return 0;
    }

    // Right drift volume: positive drift direction
    if (theTpcGeo.DriftDirection() == geo::kPosX)
    {
        if (m_useRightVolume)
            return 1;
    }

    throw pandora::StatusCodeException(pandora::STATUS_CODE_INVALID_PARAMETER);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void SBNDPandora::CreatePandoraInstances()
{
    mf::LogDebug("LArPandora") << " *** SBNDPandora::CreatePandoraInstances(...) *** " << std::endl;

    art::ServiceHandle<geo::Geometry> theGeometry;
    if (std::string::npos == theGeometry->DetectorName().find("lar1nd"))
    {
        mf::LogError("LArPandora") << " Error! Using invalid geometry: " << theGeometry->DetectorName() << std::endl;
        throw cet::exception("LArPandora") << " SBNDPandora::ConfigurePandoraGeometry --- Invalid Geometry: " << theGeometry->DetectorName();
    }

    cet::search_path sp("FW_SEARCH_PATH");
    std::string stitchingConfigFileName, configFileName;
    if (!sp.find_file(m_stitchingConfigFile, stitchingConfigFileName) || !sp.find_file(m_configFile, configFileName))
    {
        mf::LogError("LArPandora") << "   Failed to find one of: " << m_stitchingConfigFile << ", " << m_configFile << std::endl;
        throw pandora::StatusCodeException(pandora::STATUS_CODE_NOT_FOUND);
    }

    this->CreatePrimaryPandoraInstance(stitchingConfigFileName);
    this->CreateDaughterPandoraInstances(theGeometry, configFileName);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void SBNDPandora::CreatePrimaryPandoraInstance(const std::string &stitchingConfigFileName)
{
    m_pPrimaryPandora = this->CreateNewPandora();
    MultiPandoraApi::AddPrimaryPandoraInstance(m_pPrimaryPandora);
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::ReadSettings(*m_pPrimaryPandora, stitchingConfigFileName));
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, LArContent::SetLArPseudoLayerPlugin(*m_pPrimaryPandora, new lar_pandora::SBNDPseudoLayerPlugin));
}

//------------------------------------------------------------------------------------------------------------------------------------------

void SBNDPandora::CreateDaughterPandoraInstances(const art::ServiceHandle<geo::Geometry> &theGeometry, const std::string &configFileName)
{
    if (!m_pPrimaryPandora)
        throw pandora::StatusCodeException(pandora::STATUS_CODE_NOT_INITIALIZED);

    IntSet volumeIdNumbers;

    for (unsigned int icstat = 0; icstat < theGeometry->Ncryostats(); ++icstat)
    {
        for (unsigned int itpc = 0; itpc < theGeometry->NTPC(icstat); ++itpc)
        {
            try
            {
                const int volumeIdNumber(this->GetVolumeIdNumber(icstat, itpc));

                if (!volumeIdNumbers.insert(volumeIdNumber).second)
                    continue;

                const bool isForward(0 == volumeIdNumber); // ATTN: Sign of rotation matrix is taken from Volume ID
                const bool isPositiveDrift(1 == volumeIdNumber);

                std::ostringstream volumeIdString("lar1nd_");
                volumeIdString << volumeIdNumber;

                const pandora::Pandora *const pPandora = this->CreateNewPandora();
                MultiPandoraApi::AddDaughterPandoraInstance(m_pPrimaryPandora, pPandora);
                MultiPandoraApi::SetVolumeInfo(pPandora, new VolumeInfo(volumeIdNumber, volumeIdString.str(), pandora::CartesianVector(0.f, 0.f, 0.f), isPositiveDrift));
                PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, LArContent::SetLArPseudoLayerPlugin(*pPandora, new lar_pandora::SBNDPseudoLayerPlugin));
                PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, LArContent::SetLArTransformationPlugin(*pPandora, new lar_pandora::SBNDTransformationPlugin(isForward)));
                PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::ReadSettings(*pPandora, configFileName));
            }
            catch (pandora::StatusCodeException &)
            {
                mf::LogDebug("SBNDPandora") << "    No volume ID for this TPC..." << std::endl;
            }
        }
    }
}

} // namespace lar_pandora
