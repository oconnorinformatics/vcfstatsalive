#include <iostream>
#include <getopt.h>

#include <string>
#include <fstream>

#include <memory>

#ifdef VCFLIB
#include "Variant.h"
#include "BasicStatsCollector.h"
#endif

#ifdef HTSLIB
#include "lib/htslibpp/htslibpp_variant.h"
using namespace YiCppLib::HTSLibpp;
#endif

using namespace std;
using namespace VcfStatsAlive;

static struct option getopt_options[] =
{
	/* These options set a flag. */
	{"update-rate",		required_argument,	0, 'u'},
	{"first-update",	required_argument,	0, 'f'},
	{"qual-lower-val",	optional_argument,	0, 'q'},
	{"qual-upper-val",	optional_argument,	0, 'Q'},
	{"log-scale-af",	optional_argument,	0, 'l'},
	{0, 0, 0, 0}
};

static unsigned int updateRate;
static unsigned int firstUpdateRate;
static int qualHistLowerVal;
static int qualHistUpperVal;

#ifdef VCFLIB
// stats collectors are not yet adapted for HTSLIBPP types
void printStatsJansson(AbstractStatCollector* rootStatCollector);
#endif

int main(int argc, char* argv[]) {

	string filename;
	updateRate = 1000;
	firstUpdateRate = 0;
	qualHistLowerVal = 1;
	qualHistUpperVal = 200;
	bool logScaleAF = false;

	int option_index = 0;

	int ch;
	while((ch = getopt_long (argc, argv, "f:u:q:Q:l", getopt_options, &option_index)) != -1) {
		switch(ch) {
			case 0:
				break;
			case 'u':
				updateRate = strtol(optarg, NULL, 10);
				break;
			case 'f':
				firstUpdateRate = strtol(optarg, NULL, 10);
				break;
			case 'q':
				qualHistLowerVal = strtol(optarg, NULL, 10);
				if(qualHistLowerVal < 0) {
					cerr<<"Invalid quality histogram lowerbound value "<<qualHistLowerVal<<endl;
					exit(1);
				}
				break;
			case 'Q':
				qualHistUpperVal = strtol(optarg, NULL, 10);
				if(qualHistUpperVal < 0) {
					cerr<<"Invalid quality histogram upperbound value "<<qualHistUpperVal<<endl;
					exit(1);
				}
				break;
			case 'l':
				logScaleAF = true;
				break;
			default:
				break;
		}
	}

	if(qualHistUpperVal < qualHistLowerVal) {
		cerr<<"Quality histogram upperbound is lower than lowerbound"<<endl;
		exit(1);
	}

	argc -= optind;
	argv += optind;

#ifdef VCFLIB
	vcf::VariantCallFile vcfFile;
    if (argc == 0) {
        vcfFile.open(std::cin);
    }
    else {
        filename = *argv;
        vcfFile.open(filename);
    }

    if(!vcfFile.is_open()) {
        std::cerr<<"Unable to open vcf file / stream"<<std::endl;
        exit(1);
    }
#endif

#ifdef HTSLIB
    YiCppLib::HTSLibpp::htsFile htsFile;
    if (argc == 0) {
        htsFile = htsOpen("-", "r");
    }
    else {
        filename = *argv;
        htsFile = htsOpen(filename, "r");
    }
#endif


#ifdef VCFLIB
    // stats collectors are not yet adapted to HTSLIBPP types
	BasicStatsCollector *bsc = new BasicStatsCollector(qualHistLowerVal, qualHistUpperVal, logScaleAF);
#endif

	unsigned long totalVariants = 0;

#ifdef VCFLIB
    vcf::Variant var(vcfFile);

	while(vcfFile.is_open() && !vcfFile.done()) {

		vcfFile.getNextVariant(var);
		bsc->processVariant(var);
		totalVariants++;

		if((totalVariants > 0 && totalVariants % updateRate == 0) ||
				(firstUpdateRate > 0 && totalVariants >= firstUpdateRate)) {

			printStatsJansson(bsc);

			// disable first update after it has been fired.
			if(firstUpdateRate > 0) firstUpdateRate = 0;
		}
	}
    
	printStatsJansson(bsc);
#endif

#ifdef HTSLIB
    /* htslib */
    auto header = htsHeader<bcfHeader>::read(htsFile);
    std::for_each(
            htsReader<bcfRecord>::begin(htsFile, header), 
            htsReader<bcfRecord>::end(htsFile, header),
            [&totalVariants](auto &record) {
                // lambda to iterate over records
                totalVariants++;
                if((totalVariants > 0 && totalVariants % updateRate == 0) ||
                        (firstUpdateRate > 0 && totalVariants >= firstUpdateRate)) {
                    std::cout<<totalVariants<<" Records processed!"<<std::endl;
                    if(firstUpdateRate > 0) firstUpdateRate = 0;
                }
            });
#endif

#ifdef VCFLIB
	delete bsc;
#endif

	return 0;
}

#ifdef VCFLIB
void printStatsJansson(AbstractStatCollector* rootStatCollector) {

	// Create the root object that contains everything
	json_t * j_root = json_object();

	// Let the root object of the collector tree create Json
	rootStatCollector->appendJson(j_root);

	// Dump the json
	cout<<json_dumps(j_root, JSON_COMPACT | JSON_ENSURE_ASCII | JSON_PRESERVE_ORDER)<<";"<<endl;

	json_decref(j_root);
}
#endif
