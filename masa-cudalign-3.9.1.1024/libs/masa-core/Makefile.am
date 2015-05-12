AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

###############################################################################
# LIBMASA
###############################################################################

lib_LIBRARIES = libmasa.a
libmasa_a_CXXFLAGS = $(COMMONFLAGS) 

#bin_PROGRAMS = masa-cli
#masa_cli_CXXFLAGS = $(COMMONFLAGS)
#masa_cli_LDADD  = libmasa.a -lpthread
#masa_cli_SOURCES = \
#./src/masanet/MasaNetCLI.cpp

libmasa_a_SOURCES = \
./src/common/Job.cpp \
./src/common/CrosspointsFile.cpp \
./src/common/exceptions/IllegalArgumentException.cpp \
./src/common/exceptions/IOException.cpp \
./src/common/biology/Sequence.cpp \
./src/common/biology/SequenceData.cpp \
./src/common/biology/SequenceModifiers.cpp \
./src/common/biology/SequenceInfo.cpp \
./src/common/biology/Alignment.cpp \
./src/common/biology/AlignmentParams.cpp \
./src/common/biology/AlignmentBinaryFile.cpp \
./src/common/sra/SpecialRow.cpp \
./src/common/sra/SpecialRowFile.cpp \
./src/common/sra/SpecialRowRAM.cpp \
./src/common/sra/FirstRow.cpp \
./src/common/sra/SpecialRowsPartition.cpp \
./src/common/sra/SpecialRowsArea.cpp \
./src/common/Properties.cpp \
./src/common/Timer.cpp \
./src/common/RecurrentTimer.cpp \
./src/common/Status.cpp \
./src/common/BestScoreList.cpp \
./src/common/BlocksFile.cpp \
./src/common/SpecialRowReader.cpp \
./src/common/io/InitialCellsReader.cpp \
./src/common/io/FileCellsReader.cpp \
./src/common/io/FileCellsWriter.cpp \
./src/common/io/FileStream.cpp \
./src/common/io/BufferedStream.cpp \
./src/common/io/BufferedCellsReader.cpp \
./src/common/io/BufferedCellsWriter.cpp \
./src/common/io/Buffer2.cpp \
./src/common/io/BufferLogger.cpp \
./src/common/io/ReversedCellsReader.cpp \
./src/common/io/URLCellsReader.cpp \
./src/common/io/DummyCellsReader.cpp \
./src/common/io/SocketCellsReader.cpp \
./src/common/io/URLCellsWriter.cpp \
./src/common/io/DummyCellsWriter.cpp \
./src/common/io/SocketCellsWriter.cpp \
./src/common/io/TeeCellsReader.cpp \
./src/common/io/SplitCellsReader.cpp \
./src/common/AlignerPool.cpp \
./src/common/SpecialRowWriter.cpp \
./src/common/AlignerManager.cpp \
./src/common/utils.cpp \
./src/common/configs/ConfigParser.cpp \
./src/common/configs/Configs.cpp \
 \
./src/libmasa/libmasa.cpp \
./src/libmasa/libmasaTypes.hpp \
./src/libmasa/capabilities.hpp \
./src/libmasa/aligners/AbstractAligner.cpp \
./src/libmasa/aligners/AbstractAlignerSafe.cpp \
./src/libmasa/aligners/AbstractBlockAligner.cpp \
./src/libmasa/aligners/AbstractDiagonalAligner.cpp \
./src/libmasa/processors/AbstractBlockProcessor.cpp \
./src/libmasa/processors/CPUBlockProcessor.cpp \
./src/libmasa/parameters/BlockAlignerParameters.cpp \
./src/libmasa/parameters/AbstractAlignerParameters.cpp \
./src/libmasa/pruning/AbstractBlockPruning.cpp \
./src/libmasa/pruning/BlockPruningDiagonal.cpp \
./src/libmasa/pruning/BlockPruningGenericN2.cpp \
./src/libmasa/utils/AlignerUtils.cpp \
./src/libmasa/IAligner.hpp \
./src/libmasa/IManager.hpp \
./src/libmasa/IAlignerParameter.hpp \
./src/libmasa/Grid.cpp \
./src/libmasa/Partition.cpp \
 \
./src/masanet/MasaNet.cpp \
./src/masanet/Peer.cpp \
./src/masanet/PeerList.cpp \
./src/masanet/MasaNetStatus.cpp \
./src/masanet/command/Command.cpp \
./src/masanet/command/CmdJoin.cpp \
./src/masanet/command/CmdDiscover.cpp \
./src/masanet/command/CmdUndiscover.cpp \
./src/masanet/command/CmdNotifyScore.cpp \
./src/masanet/command/CmdStatusRequest.cpp \
./src/masanet/command/CmdStatusResponse.cpp \
./src/masanet/command/CmdPeerRequest.cpp \
./src/masanet/command/CmdPeerResponse.cpp \
./src/masanet/command/CmdPeerList.cpp \
./src/masanet/command/CmdCreateRing.cpp \
./src/masanet/command/CmdTestRing.cpp \
 \
./src/stage1/sw_stage1.cpp \
./src/stage2/sw_stage2.cpp \
./src/stage3/sw_stage3.cpp \
./src/stage4/sw_stage4.cpp \
./src/stage5/sw_stage5.cpp \
./src/stage6/sw_stage6.cpp

include_HEADERS = \
./src/libmasa/libmasa.hpp \
./src/libmasa/libmasaTypes.hpp \
./src/libmasa/capabilities.hpp \
./src/libmasa/aligners/AbstractAligner.hpp \
./src/libmasa/aligners/AbstractAlignerSafe.hpp \
./src/libmasa/aligners/AbstractBlockAligner.hpp \
./src/libmasa/aligners/AbstractDiagonalAligner.hpp \
./src/libmasa/processors/AbstractBlockProcessor.hpp \
./src/libmasa/processors/CPUBlockProcessor.hpp \
./src/libmasa/parameters/BlockAlignerParameters.hpp \
./src/libmasa/parameters/AbstractAlignerParameters.hpp \
./src/libmasa/pruning/AbstractBlockPruning.hpp \
./src/libmasa/pruning/BlockPruningDiagonal.hpp \
./src/libmasa/pruning/BlockPruningGenericN2.hpp \
./src/libmasa/utils/AlignerUtils.hpp \
./src/libmasa/IAligner.hpp \
./src/libmasa/IManager.hpp \
./src/libmasa/IAlignerParameter.hpp \
./src/libmasa/Grid.hpp \
./src/libmasa/Partition.hpp




noinst_HEADERS = \
./src/common/SpecialRowWriter.hpp \
./src/common/SpecialRowReader.hpp \
./src/common/Properties.hpp \
./src/common/Job.hpp \
./src/common/Common.hpp \
./src/common/Timer.hpp \
./src/common/RecurrentTimer.hpp \
./src/common/Status.hpp \
./src/common/BestScoreList.hpp \
./src/common/BlocksFile.hpp \
./src/common/biology/biology.hpp \
./src/common/biology/Sequence.hpp \
./src/common/biology/SequenceData.hpp \
./src/common/biology/SequenceModifiers.hpp \
./src/common/biology/SequenceInfo.hpp \
./src/common/biology/Alignment.hpp \
./src/common/biology/AlignmentParams.hpp \
./src/common/biology/AlignmentBinaryFile.hpp \
./src/common/biology/Constants.hpp \
./src/common/macros.hpp \
./src/common/exceptions/IllegalArgumentException.hpp \
./src/common/exceptions/IOException.hpp \
./src/common/exceptions/exceptions.hpp \
./src/common/Crosspoint.hpp \
./src/common/CrosspointsFile.hpp \
./src/common/AlignerManager.hpp \
./src/common/utils.hpp \
./src/common/sra/sra.hpp \
./src/common/sra/SpecialRow.hpp \
./src/common/sra/SpecialRowRAM.hpp \
./src/common/sra/SpecialRowFile.hpp \
./src/common/sra/FirstRow.hpp \
./src/common/sra/SpecialRowsPartition.hpp \
./src/common/sra/SpecialRowsArea.hpp \
./src/common/io/InitialCellsReader.hpp \
./src/common/io/FileCellsReader.hpp \
./src/common/io/FileCellsWriter.hpp \
./src/common/io/CellsWriter.hpp \
./src/common/io/CellsReader.hpp \
./src/common/io/SeekableCellsReader.hpp \
./src/common/io/FileStream.hpp \
./src/common/io/BufferedStream.hpp \
./src/common/io/BufferedCellsReader.hpp \
./src/common/io/BufferedCellsWriter.hpp \
./src/common/io/Buffer2.hpp \
./src/common/io/BufferLogger.hpp \
./src/common/io/ReversedCellsReader.hpp \
./src/common/io/URLCellsReader.hpp \
./src/common/io/DummyCellsReader.hpp \
./src/common/io/SocketCellsReader.hpp \
./src/common/io/URLCellsWriter.hpp \
./src/common/io/DummyCellsWriter.hpp \
./src/common/io/SocketCellsWriter.hpp \
./src/common/io/TeeCellsReader.hpp \
./src/common/io/SplitCellsReader.hpp \
./src/common/AlignerPool.hpp \
./src/common/configs/ConfigParser.hpp \
./src/common/configs/Configs.hpp \
./src/common/configs/default.cfg \
 \
./src/masanet/MasaNet.hpp \
./src/masanet/MasaNetCLI.hpp \
./src/masanet/MasaNetStatus.hpp \
./src/masanet/MasaNetCallbacks.hpp \
./src/masanet/Peer.hpp \
./src/masanet/PeerList.hpp \
./src/masanet/command/Command.hpp \
./src/masanet/command/CmdJoin.hpp \
./src/masanet/command/CmdDiscover.hpp \
./src/masanet/command/CmdUndiscover.hpp \
./src/masanet/command/CmdNotifyScore.hpp \
./src/masanet/command/CmdStatusRequest.hpp \
./src/masanet/command/CmdStatusResponse.hpp \
./src/masanet/command/CmdPeerRequest.hpp \
./src/masanet/command/CmdPeerResponse.hpp \
./src/masanet/command/CmdPeerList.hpp \
./src/masanet/command/CmdCreateRing.hpp \
./src/masanet/command/CmdTestRing.hpp \
 \
./src/libmasa/pruning/AbstractBlockPruning.hpp \
./src/libmasa/pruning/BlockPruningGenericN2.hpp \
 \
./src/stage1/sw_stage1.h \
./src/stage2/sw_stage2.h \
./src/stage3/sw_stage3.h \
./src/stage4/sw_stage4.h \
./src/stage5/sw_stage5.h \
./src/stage6/sw_stage6.h  

EXTRA_DIST = ./doxygen/masa-core.doxyfile \
./doxygen/index.html \
./doxygen/pages \
./doxygen/DoxygenLayout.xml \
./doxygen/bibtex.bib
 

BUILT_SOURCES = ./src/common/configs/default.h

noinst_DATA = ./src/common/configs/default.cfg

./src/common/configs/default.h: ./src/common/configs/default.cfg Makefile
	awk '/^[^#]/ {gsub("\t","\\t"); printf "  \"%s\",\n" , $$0}' ./src/common/configs/default.cfg > ./src/common/configs/default.h

	
mostlyclean-local:
	rm -f ./src/common/configs/default.h