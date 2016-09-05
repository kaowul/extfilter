/*
*
*    Copyright (C) Max <max1976@mail.ru>
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <map>
#include "main.h"
#include "statistictask.h"
#include "stats.h"
#include "worker.h"

static struct timeval begin_time;

static std::map<int,uint64_t> map_last_pkts;

StatisticTask::StatisticTask(int sec, std::vector<pcpp::DpdkWorkerThread*> &workerThreadVector):
	Task("StatisticTask"),
	_sec(sec),
	workerThreadVec(workerThreadVector)
{
}

static std::string formatBytes(u_int32_t howMuch)
{
	char unit = 'B';
	char buf[32];
	int buf_len=sizeof(buf);

	if(howMuch < 1024)
	{
		snprintf(buf, buf_len, "%lu %c", (unsigned long)howMuch, unit);
	} else if(howMuch < 1048576)
	{
		snprintf(buf, buf_len, "%.2f K%c", (float)(howMuch)/1024, unit);
	} else {
		float tmpGB = ((float)howMuch)/1048576;
		if(tmpGB < 1024)
		{
			snprintf(buf, buf_len, "%.2f M%c", tmpGB, unit);
		} else {
			tmpGB /= 1024;
			snprintf(buf, buf_len, "%.2f G%c", tmpGB, unit);
		}
	}
	return std::string(buf);
}

static std::string formatPackets(float numPkts)
{
	char buf[32];
	int buf_len=sizeof(buf);
	if(numPkts < 1000)
	{
		snprintf(buf, buf_len, "%.2f", numPkts);
	} else if(numPkts < 1000000)
	{
		snprintf(buf, buf_len, "%.2f K", numPkts/1000);
	} else {
		numPkts /= 1000000;
		snprintf(buf, buf_len, "%.2f M", numPkts);
	}
	return std::string(buf);
}

void StatisticTask::OutStatistic()
{
	Poco::Util::Application& app = Poco::Util::Application::instance();
	
	for(std::vector<pcpp::DpdkWorkerThread*>::iterator it=workerThreadVec.begin(); it != workerThreadVec.end(); it++)
	{
		app.logger().information("Thread on core %u statistics:", (*it)->getCoreId());
		const ThreadStats &stats=(static_cast<WorkerThread*>(*it))->getStats();
		unsigned int avg_pkt_size=0;
		struct timeval end;
		gettimeofday(&end, NULL);
		uint64_t last_pkts=0;
		std::map<int,uint64_t>::iterator it1=map_last_pkts.find((int)(*it)->getCoreId());
		if(it1 != map_last_pkts.end())
			last_pkts = it1->second;
		uint64_t tot_usec = end.tv_sec*1000000 + end.tv_usec - (begin_time.tv_sec*1000000 + begin_time.tv_usec);
		float t = (float)((stats.ip_packets-last_pkts)*1000000)/(float)tot_usec;
		map_last_pkts[(int)(*it)->getCoreId()]=stats.ip_packets;
		gettimeofday(&begin_time, NULL);
		if(stats.ip_packets && stats.total_bytes)
			avg_pkt_size = (unsigned int)(stats.total_bytes/stats.ip_packets);
		app.logger().information("Total seen packets: %" PRIu64 ", Total seen bytes: %" PRIu64 ", Average packet size: %" PRIu32 " bytes, Traffic throughput: %s pps", stats.ip_packets, stats.total_bytes, avg_pkt_size, formatPackets(t));
		app.logger().information("Total matched by ip/port: %" PRIu64 ", Total matched by ssl: %" PRIu64 ", Total matched by ssl/ip: %" PRIu64 ", Total matched by domain: %" PRIu64 ", Total matched by url: %" PRIu64, stats.matched_ip_port, stats.matched_ssl, stats.matched_ssl_ip, stats.matched_domains, stats.matched_urls);
		app.logger().information("Total redirected domains %" PRIu64 ", Total redirected urls: %" PRIu64 ", Total rst sended: %" PRIu64, stats.redirected_domains,stats.redirected_urls,stats.sended_rst);
	}
/*	app.logger().information("nDPI memory (once): %s",formatBytes(sizeof(ndpi_detection_module_struct)));
	app.logger().information("nDPI memory per flow: %s",formatBytes(nfqFilter::ndpi_size_flow_struct));
	app.logger().information("nDPI current memory usage: %s",formatBytes(nfqFilter::current_ndpi_memory));
	app.logger().information("nDPI maximum memory usage: %s",formatBytes(nfqFilter::max_ndpi_memory));

	Poco::TaskManager *pOwner=getOwner();
	if(pOwner)
	{
		Poco::TaskManager::TaskList tl=pOwner->taskList();
		for(Poco::TaskManager::TaskList::iterator it=tl.begin(); it != tl.end(); it++)
		{
			std::string threadName=(*it)->name();
			std::size_t found=threadName.find("nfqThread");
			if(found != std::string::npos)
			{
				// статистика задачи...
				struct threadStats stats;
				Poco::AutoPtr<nfqThread> p=it->cast<nfqThread>();
				p->getStats(stats);
				unsigned int avg_pkt_size=0;
				struct timeval end;
				gettimeofday(&end, NULL);
				uint64_t tot_usec = end.tv_sec*1000000 + end.tv_usec - (begin_time.tv_sec*1000000 + begin_time.tv_usec);
				float t = (float)(stats.ip_packets*1000000)/(float)tot_usec;
				if(stats.ip_packets && stats.total_bytes)
					avg_pkt_size = (unsigned int)(stats.total_bytes/stats.ip_packets);

				app.logger().information("Total seen packets: %" PRIu64 ", Total seen bytes: %" PRIu64 ", Average packet size: %" PRIu32 " bytes, Traffic throughput: %s pps", stats.ip_packets, stats.total_bytes, avg_pkt_size, formatPackets(t));
				app.logger().information("Total matched by ip/port: %" PRIu64 ", Total matched by ssl: %" PRIu64 ", Total matched by ssl/ip: %" PRIu64, stats.matched_ip_port, stats.matched_ssl, stats.matched_ssl_ip);
				app.logger().information("Total redirected domains %" PRIu64 ", Total redirected urls: %" PRIu64 ", Total marked ssl: %" PRIu64 ", Total marked hosts: %" PRIu64 ", Total rst sended: %" PRIu64, stats.redirected_domains,stats.redirected_urls,stats.marked_ssl,stats.marked_hosts,stats.sended_rst);
			}
			app.logger().debug("State of task %s is %d", (*it)->name(), (int)(*it)->state());
		}
	}*/
	
}

void StatisticTask::runTask()
{
	Poco::Util::Application& app = Poco::Util::Application::instance();
	app.logger().debug("Starting statistic task...");
	gettimeofday(&begin_time, NULL);
	int sleep_sec=_sec;
	if(!sleep_sec)
		sleep_sec=1;
	sleep_sec *= 1000;
	while (!isCancelled())
	{
		sleep(sleep_sec);
		if(_sec)
			OutStatistic();
	}
	app.logger().debug("Stopping statistic task...");
}

