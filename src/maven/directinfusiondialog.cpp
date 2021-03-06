#include "directinfusiondialog.h"
#include "directinfusionprocessor.h"

extern Database DB;

DirectInfusionDialog::DirectInfusionDialog(QWidget *parent) : QDialog(parent) {
      setupUi(this);
      setModal(true);

      //populate algorithm options
      cmbSpectralDeconvolutionAlgorithm->addItem("List All Candidates");                                    //0
      cmbSpectralDeconvolutionAlgorithm->addItem("Summarize all with Identical Fragment Matches");          //1
      cmbSpectralDeconvolutionAlgorithm->addItem("Summarize to Acyl Chains or Sum Composition");            //2

      connect(start, SIGNAL(clicked(bool)), SLOT(analyze()));
      connect(cmbSpectralDeconvolutionAlgorithm, SIGNAL(currentIndexChanged(int)), SLOT(updateSpectralCompositionDescription()));

      updateSpectralCompositionDescription();

      //Issue 226: Ms3 search GUI implemented
//      this->chkMs3IsMs3Search->setEnabled(false);
//      this->chkMs3IsMs3Search->setToolTip("MS3 search within maven gui is currently not available.");
}

DirectInfusionDialog::~DirectInfusionDialog(){
    if (directInfusionUpdate) delete(directInfusionUpdate);
}

void DirectInfusionDialog::show(){

    QStringList dbnames = DB.getLoadedDatabaseNames();
    cmbSpectralLibrary->clear();
    dbnames.push_front("ALL");
    for(QString db: dbnames ) {
        cmbSpectralLibrary->addItem(db);
    }

    QDialog::show();
}

void DirectInfusionDialog::setProgressBar(QString text, int progress, int totalSteps){
    lblProgressMsg->setText(text);
    progressBar->setRange(0, totalSteps);
    progressBar->setValue(progress);
}

void DirectInfusionDialog::updateSpectralCompositionDescription() {

    int currentIndex = cmbSpectralDeconvolutionAlgorithm->currentIndex();

    QString text("");
    switch(currentIndex){
    case 0:
        text = QString(
                    "Return all compound matches, without any attempt to determine spectral composition."
                    );
        break;
    case 1:
        text = QString(
                    "Compounds are automatically summarized to a higher level if they all contain identical fragment matches."
                    );
        break;
    case 2:
        text = QString(
                    "Compounds are automatically summarized to a higher level.\n"
                    "If they all contain identical fragment matches and have identical acyl chain isomeric forms,\n"
                    "or the same summed composition, they will be named accordingly."
                    );
        break;
    }

    txtSpecCompDescription->setText(text);

}

void DirectInfusionDialog::analyze() {

    if (DB.compoundsDB.size() == 0 || mainwindow->samples.size() == 0 || DB.getLoadedDatabaseNames().length() == 0) {
        qDebug() << "DirectInfusionDialog::analyze() could not be executed b/c no DI samples and/or compounds are loaded.";
        QDialog::hide();
        return;
    }

    if (directInfusionUpdate) {
        delete(directInfusionUpdate);
        directInfusionUpdate = nullptr;
    }

    directInfusionUpdate = new BackgroundDirectInfusionUpdate(this);

    vector<mzSample*> visibleSamples = mainwindow->getVisibleSamples();

    directInfusionUpdate->setSamples(visibleSamples);

    //Issue 238
    directInfusionUpdate->compoundDatabaseString = cmbSpectralLibrary->currentText().toStdString();

    if (directInfusionUpdate->compoundDatabaseString == "ALL") {

        //Issue 238: Explicitly remove summarized compounds associated with other searches.
        //Issue 258: Expilicity remove compounds from rumsdb before searching.

        vector<Compound*> allCompounds = DB.compoundsDB;
        allCompounds.erase(std::remove_if(allCompounds.begin(), allCompounds.end(), [](Compound *compound){return (compound->db == "summarized" || compound->db == "rumsdb");}), allCompounds.end());

        qDebug() << "DirectInfusionDialog::analyze(): Removed" << (DB.compoundsDB.size() - allCompounds.size()) << "rumsdb and summarized compounds prior to DI search.";

        directInfusionUpdate->setCompounds(allCompounds);
    } else {
        directInfusionUpdate->setCompounds(DB.getCompoundsSubset(cmbSpectralLibrary->currentText().toStdString()) );
    }

    directInfusionUpdate->setAdducts(DB.adductsDB);

    //maven version
    //Issue 335
    if(MAVEN_VERSION != "") {
        directInfusionUpdate->params->searchVersion = MAVEN_VERSION;
    }

    //ms3 search related
    directInfusionUpdate->params->ms3IsMs3Search = this->chkMs3IsMs3Search->isChecked();
    directInfusionUpdate->params->ms3MinNumMatches = this->spnMs3MinNumMatches->value();
    directInfusionUpdate->params->ms3MinNumMs3MzMatches = this->spnMs3MinNumMzMatches->value();
    directInfusionUpdate->params->ms3AnalysisMs1PrecursorPpmTolr = this->spnParTol->value();
    directInfusionUpdate->params->ms3PrecursorPpmTolr = this->spnFragTol->value();
    directInfusionUpdate->params->ms3MatchTolrInDa = static_cast<float>(this->spnMs3MatchTolrInDa->value());
    directInfusionUpdate->params->ms3MinIntensity = static_cast<float>(this->spnMs3MinIntensity->value());

    //scan filter (applies to all ms1, ms2, and ms3)
    directInfusionUpdate->params->scanFilterMinIntensity = static_cast<float>(this->spnMinIndividualMs2ScanIntensity->value());

    //consensus spectrum params (all ms levels)
    directInfusionUpdate->params->consensusIsIntensityAvgByObserved = this->chkIsIntensityAvgByObserved->isChecked();
    directInfusionUpdate->params->consensusIsNormalizeTo10K = this->chkIsNormalizeIntensityArray->isChecked();

    //Issue 268: In direct infusion runs, there may be many different kinds of SIM scans
    // eg SIM [650-750], SIM [725-825]
    // would not expect a precursor ms1 to be present in most of the different kinds of SIM scans.
    // easiest course of action is to avoid these constraints.
    // possible TODO: more specific, targeted MS1 filter strings for each individual compound

    //ms1 consensus spectrum params
    // directInfusionUpdate->params->consensusMs1PpmTolr = 10; //(default)
    //directInfusionUpdate->params->consensusMinNumMs1Scans = this->spnMinNumMs2ScansForConsensus->value();
    //directInfusionUpdate->params->consensusMinFractionMs1Scans = static_cast<float>(this->spnMinFractionMs2ScansForConsensus->value()/100.0); //displayed as a perentage

    //ms2 consensus spectrum params
    // directInfusionUpdate->params->consensusPpmTolr = 10; //(default)
    directInfusionUpdate->params->consensusMinNumMs2Scans = this->spnMinNumMs2ScansForConsensus->value();
    directInfusionUpdate->params->consensusMinFractionMs2Scans = static_cast<float>(this->spnMinFractionMs2ScansForConsensus->value()/100.0); //displayed as a perentage

    //general
    directInfusionUpdate->params->ms1IsRequireAdductPrecursorMatch = this->isRequireAdductMatch->isChecked();
    directInfusionUpdate->params->isAgglomerateAcrossSamples = this->chkAgglomerateAcrossSamples->isChecked();

    //fragment comparison
    directInfusionUpdate->params->ms2MinNumMatches = this->spnMatchXPeaks->value();
    directInfusionUpdate->params->ms2MinNumDiagnosticMatches = this->spnMatchXDiagnosticPeaks->value();
    directInfusionUpdate->params->ms2PpmTolr = this->spnFragTol->value();
    directInfusionUpdate->params->ms2MinIntensity = this->spnFragMinIntensity->value();
    directInfusionUpdate->params->ms2MinNumUniqueMatches = this->spnMatchXUniquePeaks->value();

    //precursor comparison
    directInfusionUpdate->params->ms1IsFindPrecursorIon = this->chkFindPrecursorIon->isChecked();
    directInfusionUpdate->params->ms1PpmTolr = this->spnParTol->value();
    directInfusionUpdate->params->ms1MinIntensity = this->spnParentMinIntensity->value();
    directInfusionUpdate->params->ms1ScanFilter = this->txtMs1ScanFilter->toPlainText().toStdString();

    //spectral agglomeration
    if (cmbSpectralDeconvolutionAlgorithm->currentText() == "List All Candidates"){
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::ALL_CANDIDATES;
    } else if (cmbSpectralDeconvolutionAlgorithm->currentText() == "Summarize to Acyl Chains or Sum Composition") {
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::AUTO_SUMMARIZED_ACYL_CHAINS_SUM_COMPOSITION;
    } else if (cmbSpectralDeconvolutionAlgorithm->currentText() == "Summarize all with Identical Fragment Matches") {
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::AUTO_SUMMARIZED_IDENTICAL_FRAGMENTS;
    } else if (cmbSpectralDeconvolutionAlgorithm->currentText() == "Summarize and Unique Fragments Intensity Ratio"){
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::AUTO_SUMMARIZED_MAX_THEORETICAL_INTENSITY_UNIQUE;
    }
    directInfusionUpdate->params->isReduceBySimpleParsimony = chkReduceBySimpleParsimony->isChecked();

    QString title = QString("Direct Infusion Analysis");

    if (directInfusionUpdate->params->ms3IsMs3Search) {
        title = QString("Targeted MS3 Search");
    }

    title = mainwindow->getUniquePeakTableTitle(title);

    QString encodedParams = QString(directInfusionUpdate->params->encodeParams().c_str());
    QString displayParams = QString(encodedParams);

    displayParams.replace(QRegExp(";"), "\n");
    displayParams.replace(QRegExp(DirectInfusionSearchParameters::INTERNAL_MAP_DELIMITER, Qt::CaseSensitive, QRegExp::FixedString), ",");
    displayParams.replace(QRegExp(",}", Qt::CaseSensitive, QRegExp::FixedString), "}");

    replace(displayParams.begin(), displayParams.end(), ';', '\n');
    replace(displayParams.begin(), displayParams.end(), '=', ' ');

    TableDockWidget* resultsTable = mainwindow->addPeaksTable(title, encodedParams, displayParams);
    resultsTable->setWindowTitle(title);

    connect(directInfusionUpdate, SIGNAL(newDirectInfusionAnnotation(DirectInfusionGroupAnnotation*, int)), resultsTable, SLOT(addDirectInfusionAnnotation(DirectInfusionGroupAnnotation*, int)));
    connect(directInfusionUpdate, SIGNAL(newMs3Annotation(Ms3Annotation*, int)), resultsTable, SLOT(addMs3Annotation(Ms3Annotation*, int)));
    connect(directInfusionUpdate, SIGNAL(updateProgressBar(QString,int,int)), SLOT(setProgressBar(QString, int,int)));
    connect(directInfusionUpdate, SIGNAL(closeDialog()), SLOT(hide()));
    if (directInfusionUpdate->params->ms3IsMs3Search) {
         connect(directInfusionUpdate, SIGNAL(closeDialog()), resultsTable, SLOT(showAllGroupsThenSort()));
    } else {
         connect(directInfusionUpdate, SIGNAL(closeDialog()), resultsTable, SLOT(showAllGroups()));
    }

    if ( ! directInfusionUpdate->isRunning() ) {
        directInfusionUpdate->start();	//start a background thread
    }

    //when the background thread completes, it will first close this dialog, then destroy itself.
}
