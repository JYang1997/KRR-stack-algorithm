KRR Stack Algorithm
==================================

This is a new probabilistic stack algorithm named KRR which can be used to accurately model random sampling based-LRU under arbitrary sampling size K (K-LRU).

The KRR model is first described in:
> Efficient Modeling of Random Sampling-Based LRU(ICPP'21)

The directory [src/basic_version](https://github.com/JYang1997/KRR-stack-algorithm/tree/main/src/basic_version) contains the basic model described in ICPP'21 paper.

The  ([src/KRR_mult_ops.c](https://github.com/JYang1997/KRR-stack-algorithm/blob/main/src/KRR_mult_ops.c), [inc/KRR_mult_ops.h](https://github.com/JYang1997/KRR-stack-algorithm/blob/main/inc/KRR_mult_ops.h)) is the extended version of the original KRR model:
*  The new version support variable object size miss ratio curve generation while maintain same asymptotic complexity.
*  The new version support multiple different software cache commands includes: **GET**, **SET**, **UPDATE**, **DELETE**. (In contrast, the old version follows the original stack(cache) access definition).

### compile and run example

create variable object size aware KRR

```bash
make
```
create logic(uniform) object size KRR

```bash
make UNIFORM=yes
```

### How to use KRR for mrc generation

```c
{
  //k = k-lru's sampling size K
  //stack init
  KRR_Stack_t* stack = stackInit(k);


  while (workload_not_finished)
  {

      key = //key of current kv pair
      size = //size of current kv pair
      command = //"GET", "SET", "UPDATE", "DELETE" 
      stack_distance = KRR_access(stack, key, size, command);

      //record stack distance
  }

  stackFree(stack);

  //generate MRC using stack distance distribution
}
```
[lib/src/spatial_sampling.c](https://github.com/JYang1997/KRR-stack-algorithm/blob/main/lib/src/spatial_sampling.c) shows examples of how to feed spatially filtered traces to KRR model.
