#ifndef RECURSE_CONTEXT_HPP
#define RECURSE_CONTEXT_HPP

#include <QVariant>
#include <QHash>

#include "request.hpp"
#include "response.hpp"

class Context {

public:

    Request request;
    Response response;

    // allow passing any type of QVariant
    Context &set(const QString &key, const QVariant &value) {  m_data[key] = value; return *this; };
    QVariant get(const QString &key) const { return m_data[key]; };

    // allow passing any data
    QHash<QString, void*> data;

private:
    QHash<QString, QVariant> m_data;
};

#endif
