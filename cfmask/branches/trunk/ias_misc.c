#include "input.h"
#include <string.h>
#include <stdarg.h>
#include <math.h>
 
void q_sort(int16 numbers[], int left, int right)
{
  int pivot, l_hold, r_hold;
 
  l_hold = left;
  r_hold = right;
  pivot = numbers[left];
  while (left < right)
  {
    while ((numbers[right] >= pivot) && (left < right))
      right--;
    if (left != right)
    {
      numbers[left] = numbers[right];
      left++;
    }
    while ((numbers[left] <= pivot) && (left < right))
      left++;
    if (left != right)
    {
      numbers[right] = numbers[left];
      right--;
    }
  }
  numbers[left] = pivot;
  pivot = left;
  left = l_hold;
  right = r_hold;
  if (left < pivot)
    q_sort(numbers, left, pivot-1);
  if (right > pivot)
   q_sort(numbers, pivot+1, right);
}

void quickSort(int16 numbers[], int array_size)
{
  q_sort(numbers, 0, array_size - 1);
}
 

void prctile
(
 int16 *array,
 int nums, 
 float prct,
 float *result
)
{
 int rank;
 float full_rank;
 float fraction; 

 /* Sorting the array ascendingly first */
 quickSort(array, nums);

    full_rank = (prct / 100.) * (float)(nums + 1);
 rank = floor(full_rank);
 fraction = full_rank - rank;
 if (rank == 0)
  *result = fraction * array[rank];
 else
  *result = fraction * (array[rank] - array[rank-1]) + array[rank - 1];

}

void q_sort2(float numbers[], int left, int right)
{
  int pivot, l_hold, r_hold;
 
  l_hold = left;
  r_hold = right;
  pivot = numbers[left];
  while (left < right)
  {
    while ((numbers[right] >= pivot) && (left < right))
      right--;
    if (left != right)
    {
      numbers[left] = numbers[right];
      left++;
    }
    while ((numbers[left] <= pivot) && (left < right))
      left++;
    if (left != right)
    {
      numbers[right] = numbers[left];
      right--;
    }
  }
  numbers[left] = pivot;
  pivot = left;
  left = l_hold;
  right = r_hold;
  if (left < pivot)
    q_sort2(numbers, left, pivot-1);
  if (right > pivot)
   q_sort2(numbers, pivot+1, right);
}

void quickSort2(float numbers[], int array_size)
{
  q_sort2(numbers, 0, array_size - 1);
}
 

void prctile2
(
 float *array,
 int nums, 
 float prct,
 float *result
)
{
 int rank;
 float full_rank;
 float fraction; 

 /* Sorting the array ascendingly first */
 quickSort2(array, nums);

 full_rank = (prct / 100.) * (nums + 1);
 rank = floor(full_rank);
 fraction = full_rank - rank;
 if (rank == 0)
  *result = fraction * array[rank];
 else
  *result = fraction * (array[rank] - array[rank-1]) + array[rank - 1];

}

