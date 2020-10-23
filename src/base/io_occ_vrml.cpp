/****************************************************************************
** Copyright (c) 2020, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "io_occ_vrml.h"

#include "application_item.h"
#include "caf_utils.h"
#include "document.h"
#include "math_utils.h"
#include "property_builtins.h"
#include "property_enumeration.h"
#include "task_progress.h"

#include <OSD_OpenFile.hxx>
#include <VrmlData_ShapeConvert.hxx>
#include <fstream>

namespace Mayo {
namespace IO {

namespace {

class VrmlWriterParameters : public PropertyGroup {
    MAYO_DECLARE_TEXT_ID_FUNCTIONS(Mayo::IO::OccStepReader)
public:
    VrmlWriterParameters(PropertyGroup* parentGroup)
        : PropertyGroup(parentGroup),
          shapeRepresentation(
              this, textId("shapeRepresentation"), &enumShapeRepresentation())
    {
        this->shapeRepresentation.setValue(VrmlAPI_BothRepresentation);
    }

    static const Enumeration& enumShapeRepresentation() {
        static const Enumeration values = {
            { VrmlAPI_ShadedRepresentation, textId("RepresentationShaded") },
            { VrmlAPI_WireFrameRepresentation, textId("RepresentationWireframe") },
            { VrmlAPI_BothRepresentation, textId("RepresentationBoth") },
        };
        return values;
    }

//    PropertyBool m_meshDeflectionFromShapeRelativeSize;
//    PropertyDouble m_meshDeflection;
//    PropertyDouble scale;
    PropertyEnumeration shapeRepresentation;
};

} // namespace

bool OccVrmlWriter::transfer(Span<const ApplicationItem> spanAppItem, TaskProgress* progress)
{
    m_scene.reset(new VrmlData_Scene);
    VrmlData_ShapeConvert converter(*m_scene);
    for (const ApplicationItem& appItem : spanAppItem) {
        if (appItem.isDocument()) {
            converter.ConvertDocument(appItem.document());
        }
        else if (appItem.isDocumentTreeNode()) {
            const TDF_Label label = appItem.documentTreeNode().label();
            if (XCaf::isShape(label))
                converter.AddShape(XCaf::shape(label));
        }

        const int index = &appItem - &spanAppItem.at(0);
        progress->setValue(MathUtils::mappedValue(index, 0, spanAppItem.size() - 1, 0, 100));
    }

    const auto rep = m_shapeRepresentation;
    converter.Convert(
                rep == VrmlAPI_ShadedRepresentation || rep == VrmlAPI_BothRepresentation,
                rep == VrmlAPI_WireFrameRepresentation || rep == VrmlAPI_BothRepresentation);
    return true;
}

bool OccVrmlWriter::writeFile(const QString& filepath, TaskProgress*)
{
    if (!m_scene)
        return false;

    std::ofstream outs;
    OSD_OpenStream(outs, filepath.toUtf8().constData(), std::ios::out);
    if (outs) {
        outs << *m_scene;
        outs.close();
        return outs.good();
    }

    return false;
}

std::unique_ptr<PropertyGroup> OccVrmlWriter::createParameters(PropertyGroup* parentGroup)
{
    return std::make_unique<VrmlWriterParameters>(parentGroup);
}

void OccVrmlWriter::applyParameters(const PropertyGroup* params)
{
    auto ptr = dynamic_cast<const VrmlWriterParameters*>(params);
    if (ptr) {
        this->setShapeRepresentation(ptr->shapeRepresentation.valueAs<VrmlAPI_RepresentationOfShape>());
    }
}

} // namespace IO
} // namespace Mayo
