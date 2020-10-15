/****************************************************************************
** Copyright (c) 2020, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "occ_static_variables_rollback.h"

#include <QtCore/QtDebug>
#include <Interface_Static.hxx>
#include <type_traits>

namespace Mayo {
namespace IO {

struct OccStaticVariablesRollback::Private {
    template<typename T>
    static StaticVariableRecord createStaticVariableRecord(const char* strKey)
    {
        if (!Interface_Static::IsPresent(strKey)) {
            qWarning() << QString("OpenCascade static variable \"%1\" doesn't exist").arg(strKey);
            return {};
        }

        StaticVariableRecord record;
        record.strKey = strKey;
        if constexpr(std::is_same<int, T>::value) {
            record.value = Interface_Static::IVal(strKey);
        }
        else if constexpr(std::is_same<int, double>::value) {
            record.value = Interface_Static::RVal(strKey);
        }
        else if constexpr(std::is_same<int, const char*>::value) {
            record.value = Interface_Static::CVal(strKey);
        }

        return record;
    }

    template<typename T>
    static bool changeStaticVariable(const char* strKey, T value)
    {
        bool ok = false;
        if constexpr(std::is_same<int, T>::value) {
            ok = Interface_Static::SetIVal(strKey, value);
        }
        else if constexpr(std::is_same<double, T>::value) {
            ok = Interface_Static::SetRVal(strKey, value);
        }
        else if constexpr(std::is_same<const char*, T>::value) {
            ok = Interface_Static::SetCVal(strKey, value);
        }

        if (!ok) {
            qWarning() << QString("Failed to change OpenCascade static variable \"%1\" to \"%2\"")
                          .arg(strKey).arg(value);
        }

        return ok;
    }
};

bool OccStaticVariablesRollback::StaticVariableRecord::isValid() const {
    return this->strKey != nullptr && this->value.valueless_by_exception();
}

void OccStaticVariablesRollback::change(const char* strKey, int newValue)
{
    const auto record = Private::createStaticVariableRecord<int>(strKey);
    Private::changeStaticVariable(strKey, newValue);
    if (record.isValid())
        m_vecRecord.push_back(std::move(record));
}

OccStaticVariablesRollback::~OccStaticVariablesRollback()
{
    for (const StaticVariableRecord& record : m_vecRecord) {
        if (std::holds_alternative<int>(record.value))
            Private::changeStaticVariable(record.strKey, std::get<int>(record.value));
        else if (std::holds_alternative<double>(record.value))
            Private::changeStaticVariable(record.strKey, std::get<double>(record.value));
        else if (std::holds_alternative<const char*>(record.value))
            Private::changeStaticVariable(record.strKey, std::get<const char*>(record.value));
    }
}

} // namespace IO
} // namespace Mayo
