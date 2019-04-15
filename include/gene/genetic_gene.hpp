#pragma once

template <typename GENETIC_TYPE>
class genetic_gene
{
  public:
    genetic_gene() {}
    virtual ~genetic_gene() {}
    void set_genetic_gene(GENETIC_TYPE gen)
    {
        _genetic_gen = gen;
    }
    GENETIC_TYPE get_genetic_gene()
    {
        return _genetic_gen;
    }
    GENETIC_TYPE _genetic_gen;
};

class genetic_gene_void_p : public genetic_gene<void *>
{
  public:
    genetic_gene_void_p() { _genetic_gen = NULL; }
    virtual ~genetic_gene_void_p() {}
};
