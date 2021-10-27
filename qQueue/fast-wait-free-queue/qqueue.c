#include <stdlib.h>
#include "delay.h"
#include "qqueue.h"
#include "primitives.h"

void queue_init(queue_t * q, int nprocs)
{
	q->nprocs = nprocs;
	q->head = malloc(q->nprocs * sizeof(node_t *));
	q->tail = malloc(q->nprocs * sizeof(node_t *));

	for (int i = 0; i < q->nprocs; i++)
	{
		node_t *node = malloc(sizeof(node_t));
  		node->next = NULL;

  		q->head[i] = node;
  		q->tail[i] = node;
	}
}

void queue_register(queue_t * q, handle_t * th, int id)
{
  //hzdptr_init(&th->hzd, q->nprocs, 4);
  th->id = id;
}

void enqueue(queue_t * q, handle_t * handle, void * data)
{
	node_t *node = malloc(sizeof(node_t));

	node->data = data;
	node->next = NULL;
  	node->op = 1;

  	int index = handle->id;

  	node_t *tail;
  	node_t *tail_next;

  	while (1)
	{
		//Read the queue
		tail = q->tail[index];
		tail_next = tail->next;


		/*If tail_next is null, we must lazily catch up the tail pointer
		if (tail_next != NULL)
		{
			//printf("Hey\n");
			CAS(&q->tail[index], &tail, tail_next);
			continue;
		}*/

		//Check if the queue is empty, or that there are no pending dequeue operations that need to be matched
		if (tail_next == NULL || tail->op == 1)
		{
			tail->next = node;
			q->tail[index] = node;
			break;
			/*Add our node to the list, update the tail pointer lazily
			if (CAS(&tail->next, &tail_next, node))
			{
				//Try once to update the tail pointer
				//If we fail, it means some other thread already did it
				CAS(&q->tail[index], &tail, node);
				break;
			}*/
		}
		else
		{
			node_t *head = q->head[index];
			q->head[index] = head->next;
			free(head);
			break;
		}	
	}
}

void * dequeue(queue_t * q, handle_t * handle)
{
	int index = handle->id;
	void * data = NULL;

	while (1)
	{
		/*If tail_next is null, we must lazily catch up the tail pointer
		if (tail_next != NULL)
		{
			CAS(&q->tail[index], &tail, tail_next);
			continue;
		}*/

		//Check if the queue if there are any nodes to dequeue, or that there are other dequeues waiting for a matching enqueue
		if (q->head[index]->next == NULL || q->tail[index]->op == 0)
		{
			//Add this dequeue operation to the queue as a pending operation
			node_t *node = malloc(sizeof(node_t));
			node->next = NULL;
			node->op = 0;

			q->tail[index]->next = node;
			q->tail[index] = node;
			break;
		}
		else
		{
			node_t *head = q->head[index];
			data = head->next->data;
			q->head[index] = head->next;
			free(head);
			break;
		}
	}

	return data;
}

void queue_free(int id, int nprocs) {}