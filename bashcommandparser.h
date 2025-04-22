#pragma once

#include "fuzzybashstream.h"

class BashCommandParser
{
public:
	BashCommandParser(QString command);
	QString command() const { return m_command; }
	QStringList rawArgs() const { return m_args; }

    bool hasOption(QString option);
    QString optionValue(QString option);
private:
	QString m_command;
	QStringList m_args;
};
