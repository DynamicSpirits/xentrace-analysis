	#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Event.h"
#include "EventHandler.h"
#include "Macros.h"
#include "Trace.h"
#include "list.h"
#include "Switch_sched.h"

CpuTimes cpuTimes;

int switch_sched_init(EventHandler *handler)
{
	INIT_LIST_HEAD(&(cpuTimes.cpuList));
	cpuTimes.prevNs = 0;
	cpuTimes.cpuId = 0;
	cpuTimes.prevDomId = 0;
	cpuTimes.prevVcpuId = 0;
	cpuTimes.totalCpuTime = 0;	
	memset(cpuTimes.domVcpuTimes, 0, sizeof(DomVcpuTimes)*MAX_DOMS);

	return SUCCESS;
}

int switch_sched_handler(EventHandler *handler, Event *event)
{
	unsigned int domIdIndex = event->data[0]; 
	unsigned int vcpuIdIndex = event->data[1]; 
	
	if(domIdIndex == 0x7fff)	
		domIdIndex = MAX_DOMS - 1;

	list_head *head = &(cpuTimes.cpuList);

	if(!list_empty(head))
	{
		list_head *this;

		list_for_each(this, head)
		{
			CpuTimes *tmpCpuTimes = list_entry(this, CpuTimes, cpuList);

			/* Chk if physical cpu exists on list */
			if(tmpCpuTimes->cpuId == event->cpu)
			{
				/* cpu exists and matches 
				 * add runtime to the domain and its vcpu
				 */

				unsigned long long runtime = event->ns - tmpCpuTimes->prevNs;

				/* Add runtime */
				if(	(tmpCpuTimes->prevDomId == event->data[0]) && 
					(tmpCpuTimes->prevVcpuId == event->data[1]))
				{
					tmpCpuTimes->domVcpuTimes[domIdIndex].vcpuTimes[vcpuIdIndex].totalVcpuTime += runtime;
					tmpCpuTimes->domVcpuTimes[domIdIndex].totalDomTime += runtime;
					tmpCpuTimes->totalCpuTime += runtime;
					tmpCpuTimes->prevNs = event->ns;
					
					tmpCpuTimes->prevDomId = event->data[2];
					tmpCpuTimes->prevVcpuId = event->data[3];

					return SUCCESS;
				}
				else
				{
					fprintf(stderr, "Anomaly: Prev switched dom and vcpu don't match\n");
					return FAIL;
				}
			}
		}
	}

	/* If here, means either list is empty or 
	 * list doesn't have cpu.
	 *
	 * Create new obj and add it to list.
	 */

	/* TODO: Free memory in final and reset */
	CpuTimes *newCpuTimes = (CpuTimes *) malloc(sizeof(CpuTimes));

	/* Init array to 0 */
	memset(newCpuTimes->domVcpuTimes, 0, sizeof(DomVcpuTimes)*MAX_DOMS);

	newCpuTimes->prevNs = event->ns;
	newCpuTimes->prevDomId = event->data[2];
	newCpuTimes->prevVcpuId = event->data[3];

	newCpuTimes->cpuId = event->cpu;
	newCpuTimes->domVcpuTimes[domIdIndex].domId = event->data[0];	/* DomId is the prev/switched domain */

	/* TODO: Add runtime from infprev event */
	newCpuTimes->totalCpuTime = 0;
	newCpuTimes->domVcpuTimes[domIdIndex].totalDomTime = 0;
	newCpuTimes->domVcpuTimes[domIdIndex].vcpuTimes[vcpuIdIndex].vcpuId = event->data[1];
	newCpuTimes->domVcpuTimes[domIdIndex].vcpuTimes[vcpuIdIndex].totalVcpuTime = 0;

	list_add_tail(&(newCpuTimes->cpuList), &(cpuTimes.cpuList));

	return SUCCESS;
}


int switch_sched_finalize(EventHandler *handler)
{
	CpuTimes *tmpCpuTimes;
	list_head *head = &(cpuTimes.cpuList);

	if(!list_empty(head))
	{
		int j;
		unsigned long long totalRuntime = 0;
		unsigned long long totalDomRuntime[MAX_DOMS][MAX_VCPUS+1];

		memset(totalDomRuntime, 0, sizeof(unsigned long long)*MAX_DOMS*(MAX_VCPUS+1));

		/* Iterate through all cpus */
		list_for_each_entry(tmpCpuTimes, head, cpuList)
		{
			int i, k;

			totalRuntime += tmpCpuTimes->totalCpuTime;

			for(i = 0; i < MAX_DOMS; i++)
			{
				/* index 0 has total dom time */
				totalDomRuntime[i][0] += tmpCpuTimes->domVcpuTimes[i].totalDomTime;

				for(k = 0; k < MAX_VCPUS; k++)
				{
					totalDomRuntime[i][k+1] += tmpCpuTimes->domVcpuTimes[i].vcpuTimes[k+1].totalVcpuTime;
				}
			}
		}

		tmpCpuTimes = NULL;

		/* Produce util by cpu */
		list_for_each_entry(tmpCpuTimes, head, cpuList)
		{
			printf("Physcial CPU Time Sharing:\tCPU %d = %5.2f %%\n", 
					tmpCpuTimes->cpuId,
					(float)tmpCpuTimes->totalCpuTime/totalRuntime * 100);
		}

		printf("\n");
		tmpCpuTimes = NULL;

		/* Produce util by cpu */
		list_for_each_entry(tmpCpuTimes, head, cpuList)
		{
			unsigned long long cpuUtil = tmpCpuTimes->totalCpuTime - tmpCpuTimes->domVcpuTimes[MAX_DOMS - 1].totalDomTime;
			printf("Physcial CPU Utilization:\tCPU %d = %5.2f %%\n", 
					tmpCpuTimes->cpuId,
					(float)cpuUtil/totalRuntime * 100);
		}

		printf("\n");

		/* Produce util by Domains */
		for(j = 0; j < MAX_DOMS; j++)
		{
			int k;

			if(totalDomRuntime[j][0])
			{
				printf("Domain CPU Utilization:\t\tDom %d = %5.2f %%\n",
						(j == (MAX_DOMS - 1)) ? 0x7fff : j,
						(float)totalDomRuntime[j][0]/totalRuntime * 100);

				for(k = 0; k < MAX_VCPUS - 1; k++)
				{
					if(totalDomRuntime[j][k+1])
					{
						printf("\tVCPU Utilization: VCPU %d = %5.2f %%\n",
								k,
								(float)totalDomRuntime[j][k+1]/totalDomRuntime[j][0] * 100);
					}
				}
			}
		}

		tmpCpuTimes = NULL;

		/* Free malloc'd memory */
		list_for_each_entry_reverse(tmpCpuTimes, head, cpuList)
		{
			free(tmpCpuTimes);
		}
	}
	
	return 0;
}

void switch_sched_reset(void)
{
	CpuTimes *tmpCpuTimes;
	list_head *head = &(cpuTimes.cpuList);
	
	/* Free malloc'd memory */
	list_for_each_entry_reverse(tmpCpuTimes, head, cpuList)
	{
		free(tmpCpuTimes);
	}

	INIT_LIST_HEAD(&(cpuTimes.cpuList));
	cpuTimes.prevNs = 0;
	cpuTimes.cpuId = 0;
	cpuTimes.prevDomId = 0;
	cpuTimes.prevVcpuId = 0;
	cpuTimes.totalCpuTime = 0;	
	memset(cpuTimes.domVcpuTimes, 0, sizeof(DomVcpuTimes)*MAX_DOMS);

}

struct EventHandler switchSchedHandler =
{
	.name = "__enter_scheduler",
	.event_id =TRC_SCHED_SWITCH,
	.init = switch_sched_init,
	.data = (void*)&cpuTimes,
	.process_event = switch_sched_handler,
	.finalize = switch_sched_finalize,
};