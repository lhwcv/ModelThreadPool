# ModelThreadPool
c++11 thread pool for DNN models

``` C++
#include <iostream>
#include "model_thread_pool.h"

int main()
{
    hpc::Model_ThreadPool ocrThreadPool(2);
    std::vector<std::future<hpc::TextLineResult> > rets;
    std::vector<std::vector<std::string>> modelCfgs(2);
    ocrThreadPool.load_models(modelCfgs);

    for(int j = 1; j < 11; j++)
    {
        hpc::Image  image0;
        image0.w =  j*10;
        image0.h =  32;
        rets.emplace_back(ocrThreadPool.enqueue(image0));

    }
    std::vector<hpc::TextLineResult> barrierResults;
    for(std::future<hpc::TextLineResult> &r: rets)
    {
        barrierResults.emplace_back(r.get());
    }
    for(auto &r : barrierResults)
    {
        std::cout<<r.text<<std::endl;
    }

    return 0;
}
 ```
 
 ref:  https://github.com/progschj/ThreadPool
