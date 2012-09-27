/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Export
Copyright 2012 (c)  All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

Disclaimer: THIS SOFTWARE IS PROVIDED BY  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
IN NO EVENT SHALL  THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#include "partioVisualizer.h"
#include "partioInstancer.h"
#include "partioEmitter.h"
#include "partioExport.h"
#include "partioImport.h"
#include "partio4MayaShared.h"
#include <maya/MFnPlugin.h>



MStatus initializePlugin ( MObject obj )
{

    // source  mel scripts this way if they're missing from the script path it will alert the user...
    MGlobal::executeCommand("source AEpartioEmitterTemplate.mel");
    MGlobal::executeCommand("source AEpartioVisualizerTemplate.mel");
    MGlobal::executeCommand("source AEpartioInstancerTemplate.mel");
    MGlobal::executeCommand("source partioExportGui.mel");
    MGlobal::executeCommand("source partioUtils.mel");

    MStatus status;
    MFnPlugin plugin ( obj, "RedpawFX,Luma Pictures,WDAS", "0.9.3b(sigg)", "Any" );

    status = plugin.registerShape( "partioVisualizer", partioVisualizer::id,
                                   &partioVisualizer::creator,
                                   &partioVisualizer::initialize,
                                   &partioVisualizerUI::creator);


    if ( !status )
    {
        status.perror ( "registerNode partioVisualizer failed" );
        return status;
    }
    status = plugin.registerShape( "partioInstancer", partioInstancer::id,
                                   &partioInstancer::creator,
                                   &partioInstancer::initialize,
                                   &partioInstancerUI::creator);


    if ( !status )
    {
        status.perror ( "registerNode partioInstancer failed" );
        return status;
    }


    status = plugin.registerNode ( "partioEmitter", partioEmitter::id,
                                   &partioEmitter::creator, &partioEmitter::initialize,
                                   MPxNode::kEmitterNode );
    if ( !status )
    {
        status.perror ( "registerNode partioEmitter failed" );
        return status;
    }

    status = plugin.registerCommand("partioExport",PartioExport::creator, PartioExport::createSyntax );
    if (!status)
    {
        status.perror("registerCommand partioExport failed");
        return status;
    }

    status = plugin.registerCommand("partioImport",PartioImport::creator, PartioImport::createSyntax );
    if (!status)
    {
        status.perror("registerCommand partioImport failed");
    }

    return status;
}

MStatus uninitializePlugin ( MObject obj )
{
    MStatus status;
    MFnPlugin plugin ( obj );

    status = plugin.deregisterNode ( partioVisualizer::id );
    if ( !status )
    {
        status.perror ( "deregisterNode partioVisualizer failed" );
        return status;
    }
    status = plugin.deregisterNode ( partioInstancer::id );
    if ( !status )
    {
        status.perror ( "deregisterNode partioInstancer failed" );
        return status;
    }

    status = plugin.deregisterNode ( partioEmitter::id );
    if ( !status )
    {
        status.perror ( "deregisterNode partioEmitter failed" );
        return status;
    }

    status = plugin.deregisterCommand("partioExport");
    if (!status)
    {
        status.perror("deregisterCommand partioExport failed");
        return status;
    }
    status = plugin.deregisterCommand("partioImport");
    if (!status)
    {
        status.perror("deregisterCommand partioImport failed");
    }
    return status;

}
