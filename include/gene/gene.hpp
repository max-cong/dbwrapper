#pragma once
namespace gene
{
template <typename GENETIC_TYPE = void *>
class gene
{
  public:
	gene() {}
	virtual ~gene() {}
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
} // namespace gene