import React, { useState, useEffect } from 'react';
import { 
  Activity, 
  Satellite, 
  Vote, 
  Calendar, 
  RefreshCw, 
  ShieldCheck, 
  AlertCircle, 
  CheckCircle2, 
  Clock,
  Server,
  Globe,
  ChevronRight,
  Search
} from 'lucide-react';

const UrnaSatelliteSystem = () => {
  const [activeTab, setActiveTab] = useState('realtime');
  const [isConnecting, setIsConnecting] = useState(false);
  const [connectionStatus, setConnectionStatus] = useState('disconnected');
  const [searchQuery, setSearchQuery] = useState('');
  const [notifications, setNotifications] = useState([
    { id: 1, msg: 'Sincronização de satélite via Banda Ka concluída', type: 'success', time: '10:42' },
    { id: 2, msg: 'Tentativa de acesso não autorizado bloqueada no Nodo 04', type: 'error', time: '11:15' },
  ]);

  const [realtimeProcesses, setRealtimeProcesses] = useState([
    { id: 'PROC-8821', region: 'Norte', status: 'Sincronizando', progress: 65, load: '12%', latency: '140ms' },
    { id: 'PROC-9012', region: 'Sul', status: 'Ativo', progress: 100, load: '45%', latency: '85ms' },
    { id: 'PROC-7734', region: 'Nordeste', status: 'Aguardando', progress: 10, load: '2%', latency: '210ms' },
    { id: 'PROC-4410', region: 'Centro-Oeste', status: 'Sincronizando', progress: 32, load: '18%', latency: '110ms' },
  ]);

  const [scheduledUpdates, setScheduledUpdates] = useState([
    { id: 'SCH-101', process: 'Atualização Firmware Urnas v4.2', date: '2026-10-26', time: '02:00', priority: 'Alta', status: 'Agendado' },
    { id: 'SCH-102', process: 'Backup de Logs Satelitais', date: '2026-10-27', time: '04:00', priority: 'Média', status: 'Pendente' },
    { id: 'SCH-103', process: 'Recalibração de Antenas Terrestres', date: '2026-10-28', time: '23:00', priority: 'Baixa', status: 'Agendado' },
  ]);

  const handleConnect = () => {
    setIsConnecting(true);
    setTimeout(() => {
      setConnectionStatus('connected');
      setIsConnecting(false);
    }, 2000);
  };

  const addSchedule = (e) => {
    e.preventDefault();
    const formData = new FormData(e.target);
    const newUpdate = {
      id: `SCH-${Math.floor(Math.random() * 1000)}`,
      process: formData.get('process'),
      date: formData.get('date'),
      time: formData.get('time'),
      priority: formData.get('priority'),
      status: 'Agendado'
    };
    setScheduledUpdates([...scheduledUpdates, newUpdate]);
    e.target.reset();
  };

  return (
    <div className="min-h-screen bg-slate-950 text-slate-200 font-sans p-4 lg:p-8">
      {/* Header de Status Global */}
      <header className="flex flex-col md:flex-row justify-between items-start md:items-center gap-4 mb-8 bg-slate-900 p-6 rounded-2xl border border-slate-800 shadow-2xl">
        <div className="flex items-center gap-4">
          <div className={`p-3 rounded-xl ${connectionStatus === 'connected' ? 'bg-emerald-500/20 text-emerald-400' : 'bg-rose-500/20 text-rose-400'} transition-colors`}>
            <Server size={32} />
          </div>
          <div>
            <h1 className="text-2xl font-bold tracking-tight text-white">Nexus Urna-Sat</h1>
            <p className="text-slate-400 text-sm flex items-center gap-2">
              <Globe size={14} /> Interface de Comando e Sincronização Global
            </p>
          </div>
        </div>

        <div className="flex items-center gap-3">
          <div className="text-right hidden sm:block">
            <p className="text-xs text-slate-500 uppercase font-bold">Status do Link</p>
            <p className={`text-sm font-medium ${connectionStatus === 'connected' ? 'text-emerald-400' : 'text-rose-400'}`}>
              {connectionStatus === 'connected' ? 'Sincronizado via Satélite' : 'Desconectado'}
            </p>
          </div>
          <button 
            onClick={handleConnect}
            disabled={isConnecting || connectionStatus === 'connected'}
            className={`px-6 py-2 rounded-lg font-semibold transition-all flex items-center gap-2 ${
              connectionStatus === 'connected' 
              ? 'bg-slate-800 text-slate-500 cursor-not-allowed' 
              : 'bg-indigo-600 hover:bg-indigo-500 text-white shadow-lg shadow-indigo-500/20'
            }`}
          >
            {isConnecting ? <RefreshCw className="animate-spin" size={18} /> : <ShieldCheck size={18} />}
            {connectionStatus === 'connected' ? 'Conectado' : 'Estabelecer Link'}
          </button>
        </div>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-12 gap-8">
        {/* Sidebar de Navegação e Notificações */}
        <aside className="lg:col-span-3 space-y-6">
          <nav className="bg-slate-900 p-2 rounded-2xl border border-slate-800">
            <button 
              onClick={() => setActiveTab('realtime')}
              className={`w-full flex items-center gap-3 px-4 py-3 rounded-xl transition-all ${activeTab === 'realtime' ? 'bg-indigo-600 text-white shadow-lg' : 'text-slate-400 hover:bg-slate-800'}`}
            >
              <Activity size={20} />
              <span className="font-medium">Tempo Real</span>
            </button>
            <button 
              onClick={() => setActiveTab('scheduling')}
              className={`w-full flex items-center gap-3 px-4 py-3 rounded-xl transition-all mt-2 ${activeTab === 'scheduling' ? 'bg-indigo-600 text-white shadow-lg' : 'text-slate-400 hover:bg-slate-800'}`}
            >
              <Calendar size={20} />
              <span className="font-medium">Agendamentos</span>
            </button>
          </nav>

          <div className="bg-slate-900 p-6 rounded-2xl border border-slate-800">
            <h3 className="text-sm font-bold text-slate-500 uppercase mb-4 flex items-center gap-2">
              <AlertCircle size={16} /> Log de Eventos
            </h3>
            <div className="space-y-4">
              {notifications.map(n => (
                <div key={n.id} className="text-xs border-l-2 border-slate-700 pl-3 py-1">
                  <div className="flex justify-between mb-1">
                    <span className={n.type === 'success' ? 'text-emerald-400' : 'text-rose-400'}>
                      {n.type === 'success' ? 'INFO' : 'ALERTA'}
                    </span>
                    <span className="text-slate-600">{n.time}</span>
                  </div>
                  <p className="text-slate-300">{n.msg}</p>
                </div>
              ))}
            </div>
          </div>
        </aside>

        {/* Área Principal de Conteúdo */}
        <main className="lg:col-span-9">
          {activeTab === 'realtime' ? (
            <div className="space-y-6 animate-in fade-in slide-in-from-bottom-4 duration-500">
              <div className="flex flex-col sm:flex-row justify-between items-start sm:items-center gap-4">
                <h2 className="text-xl font-bold text-white flex items-center gap-2">
                  <Vote className="text-indigo-400" /> Monitoramento de Processos Ativos
                </h2>
                <div className="relative w-full sm:w-64">
                  <Search className="absolute left-3 top-1/2 -translate-y-1/2 text-slate-500" size={16} />
                  <input 
                    type="text" 
                    placeholder="Filtrar processo..." 
                    className="w-full bg-slate-900 border border-slate-800 rounded-lg py-2 pl-10 pr-4 text-sm focus:outline-none focus:ring-2 focus:ring-indigo-500 transition-all"
                    value={searchQuery}
                    onChange={(e) => setSearchQuery(e.target.value)}
                  />
                </div>
              </div>

              <div className="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-4 gap-4">
                {realtimeProcesses
                  .filter(p => p.id.toLowerCase().includes(searchQuery.toLowerCase()) || p.region.toLowerCase().includes(searchQuery.toLowerCase()))
                  .map(p => (
                  <div key={p.id} className="bg-slate-900 p-5 rounded-2xl border border-slate-800 hover:border-indigo-500/50 transition-all group">
                    <div className="flex justify-between items-start mb-4">
                      <span className="text-xs font-mono text-indigo-400 bg-indigo-400/10 px-2 py-1 rounded">{p.id}</span>
                      <div className={`h-2 w-2 rounded-full animate-pulse ${p.status === 'Ativo' ? 'bg-emerald-500' : 'bg-amber-500'}`} />
                    </div>
                    <h4 className="text-lg font-bold text-white mb-1">{p.region}</h4>
                    <p className="text-slate-400 text-xs mb-4">{p.status}</p>
                    
                    <div className="space-y-3">
                      <div>
                        <div className="flex justify-between text-[10px] uppercase font-bold text-slate-500 mb-1">
                          <span>Sincronização</span>
                          <span>{p.progress}%</span>
                        </div>
                        <div className="h-1.5 w-full bg-slate-800 rounded-full overflow-hidden">
                          <div 
                            className="h-full bg-indigo-500 transition-all duration-1000" 
                            style={{ width: `${p.progress}%` }} 
                          />
                        </div>
                      </div>
                      <div className="flex justify-between items-center pt-2 border-t border-slate-800">
                        <div className="flex items-center gap-1 text-[10px] text-slate-500">
                          <Activity size={12} /> {p.load}
                        </div>
                        <div className="flex items-center gap-1 text-[10px] text-slate-500">
                          <Clock size={12} /> {p.latency}
                        </div>
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          ) : (
            <div className="space-y-6 animate-in fade-in slide-in-from-bottom-4 duration-500">
              <div className="flex justify-between items-center">
                <h2 className="text-xl font-bold text-white flex items-center gap-2">
                  <Satellite className="text-indigo-400" /> Agendamento de Atualizações
                </h2>
              </div>

              <div className="grid grid-cols-1 xl:grid-cols-3 gap-8">
                {/* Formulário de Agendamento */}
                <div className="bg-slate-900 p-6 rounded-2xl border border-slate-800 h-fit">
                  <h3 className="text-sm font-bold text-slate-400 uppercase mb-6">Novo Agendamento</h3>
                  <form onSubmit={addSchedule} className="space-y-4">
                    <div>
                      <label className="block text-xs font-medium text-slate-500 mb-1">Nome do Processo</label>
                      <input name="process" required type="text" className="w-full bg-slate-950 border border-slate-800 rounded-lg p-2 text-sm focus:ring-2 focus:ring-indigo-500 outline-none" placeholder="Ex: Patch de Segurança v1.1" />
                    </div>
                    <div className="grid grid-cols-2 gap-4">
                      <div>
                        <label className="block text-xs font-medium text-slate-500 mb-1">Data</label>
                        <input name="date" required type="date" className="w-full bg-slate-950 border border-slate-800 rounded-lg p-2 text-sm focus:ring-2 focus:ring-indigo-500 outline-none" />
                      </div>
                      <div>
                        <label className="block text-xs font-medium text-slate-500 mb-1">Hora</label>
                        <input name="time" required type="time" className="w-full bg-slate-950 border border-slate-800 rounded-lg p-2 text-sm focus:ring-2 focus:ring-indigo-500 outline-none" />
                      </div>
                    </div>
                    <div>
                      <label className="block text-xs font-medium text-slate-500 mb-1">Prioridade</label>
                      <select name="priority" className="w-full bg-slate-950 border border-slate-800 rounded-lg p-2 text-sm focus:ring-2 focus:ring-indigo-500 outline-none">
                        <option>Baixa</option>
                        <option>Média</option>
                        <option>Alta</option>
                        <option>Crítica</option>
                      </select>
                    </div>
                    <button type="submit" className="w-full bg-indigo-600 hover:bg-indigo-500 text-white font-bold py-2 rounded-lg transition-all flex items-center justify-center gap-2 shadow-lg shadow-indigo-500/20">
                      <CheckCircle2 size={18} /> Confirmar Agendamento
                    </button>
                  </form>
                </div>

                {/* Lista de Agendamentos */}
                <div className="xl:col-span-2 bg-slate-900 rounded-2xl border border-slate-800 overflow-hidden">
                  <table className="w-full text-left border-collapse">
                    <thead>
                      <tr className="bg-slate-800/50 text-slate-400 text-xs uppercase font-bold">
                        <th className="p-4">ID / Processo</th>
                        <th className="p-4">Data/Hora</th>
                        <th className="p-4">Prioridade</th>
                        <th className="p-4">Status</th>
                        <th className="p-4">Ação</th>
                      </tr>
                    </thead>
                    <tbody className="text-sm divide-y divide-slate-800">
                      {scheduledUpdates.map(s => (
                        <tr key={s.id} className="hover:bg-slate-800/30 transition-colors group">
                          <td className="p-4">
                            <div className="font-mono text-indigo-400 text-xs">{s.id}</div>
                            <div className="font-medium text-white">{s.process}</div>
                          </td>
                          <td className="p-4 text-slate-400">
                            <div className="flex items-center gap-2">
                              <Calendar size={14} /> {s.date} {s.time}
                            </div>
                          </td>
                          <td className="p-4">
                            <span className={`px-2 py-1 rounded-full text-[10px] font-bold uppercase ${
                              s.priority === 'Alta' ? 'bg-rose-500/20 text-rose-400' : 
                              s.priority === 'Média' ? 'bg-amber-500/20 text-amber-400' : 
                              'bg-slate-700/50 text-slate-400'
                            }`}>
                              {s.priority}
                            </span>
                          </td>
                          <td className="p-4">
                            <div className="flex items-center gap-2 text-emerald-400">
                              <div className="h-1.5 w-1.5 rounded-full bg-emerald-400" />
                              {s.status}
                            </div>
                          </td>
                          <td className="p-4">
                            <button className="p-2 text-slate-500 hover:text-white transition-colors">
                              <ChevronRight size={18} />
                            </button>
                          </td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              </div>
            </div>
          )}
        </main>
      </div>
    </div>
  );
};

export default UrnaSatelliteSystem;