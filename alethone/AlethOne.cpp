/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file AlethOne.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "AlethOne.h"
#include <QSettings>
#include <QTimer>
#include <QTime>
#include <QClipboard>
#include <QToolButton>
#include <QButtonGroup>
#include <QDateTime>
#include <QDesktopServices>
#include <libethcore/Common.h>
#include <libethcore/ICAP.h>
#include <libethereum/Client.h>
#include <libethashseal/EthashClient.h>
#include <libwebthree/WebThree.h>
#include <libaleth/SendDialog.h>
#include "alethzero/BuildInfo.h"
#include "ui_AlethOne.h"

using namespace std;
using namespace dev;
using namespace p2p;
using namespace eth;
using namespace aleth;
using namespace one;

AlethOne::AlethOne():
	m_ui(new Ui::AlethOne)
{
	g_logVerbosity = 1;

	setWindowFlags(Qt::Window);
	m_ui->setupUi(this);
	m_aleth.init(Aleth::Nothing, "winze.io", "anon");

	m_ui->version->setText(QString("winze.io v") + "0.0.2");
	m_ui->sync->setAleth(&m_aleth);

	{
		QTimer* t = new QTimer(this);
		connect(t, SIGNAL(timeout()), SLOT(refresh()));
		t->start(1000);
	}

	QSettings s("ethereum", "alethone");
	QString url = "http://pool.winze.io/?miner=7@0x" + QString::fromStdString(m_aleth.keyManager().accounts().front().hex());
	s.setValue("url", url);
	// TODO Set MH/s after a test

	refresh();
}

AlethOne::~AlethOne()
{
	delete m_ui;
}

void AlethOne::refresh()
{
	m_ui->peers->setValue(m_aleth ? m_aleth.web3()->peerCount() : 0);
	bool s = m_aleth ? m_aleth.ethereum()->isSyncing() : false;
	pair<uint64_t, unsigned> gp = EthashAux::fullGeneratingProgress();
	m_ui->stack->setCurrentIndex(1);
	if (m_ui->mining->isChecked())
	{
		m_ui->mining->setText(tr("Stop Mining "));
		bool m;
		u256 r;
		if (m_aleth)
		{
			log("is m_aleth?");
			try
			{
				m = asEthashClient(m_aleth.ethereum())->isMining();
				r = asEthashClient(m_aleth.ethereum())->hashrate();
				log("isMining()");
			}
			catch (...)
			{
				m = 0;
				r = 0;
			}
		}
		else
		{
			m = m_slave.isWorking();
			r = m ? m_slave.hashrate() : 0;
		}
		if (r > 0)
		{
			QStringList ss = QString::fromStdString(inUnits(r, { "hash/s", "Khash/s", "Mhash/s", "Ghash/s" })).split(" ");
			m_ui->hashrate->setText(ss[0]);
			m_ui->underHashrate->setText(ss[1]);
		}
		else if (gp.first != EthashAux::NotGenerating)
		{
			m_ui->hashrate->setText(QString("%1%").arg(gp.second));
			m_ui->underHashrate->setText(QString(tr("Preparing...")).arg(gp.first));
		}
		else if (s) {
			m_ui->stack->setCurrentIndex(0);
			log("setCurrentIndex(0)");
		}
		else if (m)
		{
			m_ui->hashrate->setText("...");
			m_ui->underHashrate->setText(tr("Preparing..."));
		}
		else
		{
			m_ui->hashrate->setText("...");
			m_ui->underHashrate->setText(tr("Waiting..."));
		}
	}
	else
	{
		m_ui->mining->setText(tr("Start winze!"));
		if (s)
			m_ui->stack->setCurrentIndex(0);
		else
		{
			m_ui->hashrate->setText("/");
			m_ui->underHashrate->setText("Not mining");
		}
	}
}

void AlethOne::on_feedback_clicked()
{
	QString link = "http://winze.io/feedback?miner=7@0x" + QString::fromStdString(m_aleth.keyManager().accounts().front().hex());
	QDesktopServices::openUrl(QUrl(link));
 }

void AlethOne::on_mining_toggled(bool _on)
{
	if (_on)
	{
#ifdef NDEBUG
		char const* sealer = "opencl";
#else
		char const* sealer = "cpu";
#endif
		QSettings s("ethereum", "alethone");
		m_slave.setURL(s.value("url").toString());
		m_slave.setSealer(sealer);
	}
	else
	{
		if (m_aleth)
			m_aleth.ethereum()->stopSealing();
		m_slave.stop();
	}
	refresh();
}

void AlethOne::log(QString _s)
{
	QString s;
	s = QTime::currentTime().toString("hh:mm:ss") + " " + _s;
	m_ui->log->appendPlainText(s);
}
